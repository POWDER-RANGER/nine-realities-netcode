/**
 * Client-Server Network Protocol for Nine Realities Netcode
 * 
 * Implements a binary protocol for efficient communication between
 * clients and authoritative server. Designed for:
 * - Low-latency input transmission
 * - Efficient state snapshot broadcasting
 * - Reliable delivery of critical packets
 * - Bandwidth optimization
 * 
 * Message Types:
 * - CLIENT_HELLO: Connection initiation
 * - INPUT: Player input with timestamp and tick
 * - TIME_SYNC_REQ/RES: Clock synchronization
 * - SNAPSHOT: Full or delta state update
 * - ACK: Acknowledgment of received packets
 * - DISCONNECT: Graceful disconnect
 */

const MessageType = {
  // Client → Server
  CLIENT_HELLO: 0x01,
  CLIENT_INPUT: 0x02,
  TIME_SYNC_REQ: 0x03,
  CLIENT_ACK: 0x04,
  CLIENT_DISCONNECT: 0x05,
  
  // Server → Client
  SERVER_HELLO: 0x10,
  SERVER_SNAPSHOT: 0x11,
  TIME_SYNC_RES: 0x12,
  SERVER_ACK: 0x13,
  SERVER_KICK: 0x14
};

class NetworkProtocol {
  constructor() {
    // Packet tracking for reliability
    this.nextSequenceId = 0;
    this.pendingAcks = new Map(); // sequenceId → { packet, timestamp, retries }
    this.receivedSequences = new Set(); // Avoid duplicate processing
    
    // Configuration
    this.maxRetries = 3;
    this.ackTimeout = 100; // ms
    this.maxPacketSize = 1400; // bytes (below MTU)
    
    // Statistics
    this.bytesSent = 0;
    this.bytesReceived = 0;
    this.packetsLost = 0;
  }

  /**
   * Create CLIENT_HELLO packet
   * @param {string} playerId - Unique player identifier
   * @param {string} playerName - Display name
   * @returns {Object} Hello packet
   */
  createClientHello(playerId, playerName) {
    return this.createPacket(MessageType.CLIENT_HELLO, {
      playerId,
      playerName,
      protocolVersion: 1,
      timestamp: Date.now()
    });
  }

  /**
   * Create CLIENT_INPUT packet
   * @param {number} tick - Client tick number
   * @param {Object} input - Input data
   * @param {number} timestamp - Local timestamp
   * @returns {Object} Input packet
   */
  createInput(tick, input, timestamp) {
    return this.createPacket(MessageType.CLIENT_INPUT, {
      tick,
      input,
      timestamp
    }, true); // Requires acknowledgment
  }

  /**
   * Create SERVER_SNAPSHOT packet
   * @param {number} tick - Server tick
   * @param {Object} state - Full or delta state
   * @param {boolean} isDelta - Whether this is a delta update
   * @param {number} lastAckedInput - Last input sequence received from client
   * @returns {Object} Snapshot packet
   */
  createSnapshot(tick, state, isDelta = false, lastAckedInput = 0) {
    return this.createPacket(MessageType.SERVER_SNAPSHOT, {
      tick,
      state,
      isDelta,
      lastAckedInput,
      timestamp: Date.now()
    });
  }

  /**
   * Create TIME_SYNC_REQ packet
   * @returns {Object} Sync request packet
   */
  createTimeSyncRequest() {
    return this.createPacket(MessageType.TIME_SYNC_REQ, {
      clientSendTime: Date.now()
    });
  }

  /**
   * Create TIME_SYNC_RES packet
   * @param {number} clientSendTime - Original client timestamp
   * @param {number} serverTick - Current server tick
   * @returns {Object} Sync response packet
   */
  createTimeSyncResponse(clientSendTime, serverTick) {
    const now = Date.now();
    return this.createPacket(MessageType.TIME_SYNC_RES, {
      clientSendTime,
      serverReceiveTime: now,
      serverSendTime: now,
      serverTick
    });
  }

  /**
   * Create ACK packet
   * @param {number} sequenceId - Sequence to acknowledge
   * @returns {Object} ACK packet
   */
  createAck(sequenceId) {
    return {
      type: MessageType.CLIENT_ACK,
      sequenceId,
      timestamp: Date.now()
    };
  }

  /**
   * Create base packet structure
   * @param {number} type - Message type
   * @param {Object} payload - Packet data
   * @param {boolean} requiresAck - Whether packet needs acknowledgment
   * @returns {Object} Packet
   */
  createPacket(type, payload, requiresAck = false) {
    const packet = {
      type,
      sequenceId: this.nextSequenceId++,
      payload,
      requiresAck,
      timestamp: Date.now()
    };
    
    // Track for potential retransmission
    if (requiresAck) {
      this.pendingAcks.set(packet.sequenceId, {
        packet,
        timestamp: packet.timestamp,
        retries: 0
      });
    }
    
    return packet;
  }

  /**
   * Process received packet
   * @param {Object} packet - Received packet
   * @returns {Object|null} Processed payload or null if duplicate
   */
  processPacket(packet) {
    // Check for duplicate
    if (this.receivedSequences.has(packet.sequenceId)) {
      console.warn(`Duplicate packet ${packet.sequenceId} ignored`);
      return null;
    }
    
    this.receivedSequences.add(packet.sequenceId);
    
    // Clean old sequences (keep last 1000)
    if (this.receivedSequences.size > 1000) {
      const sorted = Array.from(this.receivedSequences).sort((a, b) => a - b);
      for (let i = 0; i < 500; i++) {
        this.receivedSequences.delete(sorted[i]);
      }
    }
    
    // Send ACK if required
    if (packet.requiresAck) {
      return {
        payload: packet.payload,
        ackPacket: this.createAck(packet.sequenceId)
      };
    }
    
    return { payload: packet.payload };
  }

  /**
   * Process ACK packet
   * @param {number} sequenceId - Acknowledged sequence
   */
  processAck(sequenceId) {
    if (this.pendingAcks.has(sequenceId)) {
      this.pendingAcks.delete(sequenceId);
      console.log(`✓ Packet ${sequenceId} acknowledged`);
    }
  }

  /**
   * Check for packets that need retransmission
   * @returns {Array} Packets to retransmit
   */
  getRetransmissions() {
    const now = Date.now();
    const toRetransmit = [];
    
    for (const [seqId, pending] of this.pendingAcks.entries()) {
      const elapsed = now - pending.timestamp;
      
      if (elapsed > this.ackTimeout) {
        if (pending.retries < this.maxRetries) {
          pending.retries++;
          pending.timestamp = now;
          toRetransmit.push(pending.packet);
          console.warn(`⚠ Retransmitting packet ${seqId} (attempt ${pending.retries})`);
        } else {
          // Give up
          this.pendingAcks.delete(seqId);
          this.packetsLost++;
          console.error(`✗ Packet ${seqId} lost after ${this.maxRetries} retries`);
        }
      }
    }
    
    return toRetransmit;
  }

  /**
   * Serialize packet to binary (simplified - uses JSON for demo)
   * In production, use proper binary serialization (e.g., MessagePack, Protocol Buffers)
   * @param {Object} packet - Packet to serialize
   * @returns {Buffer|Uint8Array} Serialized data
   */
  serialize(packet) {
    const json = JSON.stringify(packet);
    this.bytesSent += json.length;
    return Buffer.from(json, 'utf8');
  }

  /**
   * Deserialize binary packet
   * @param {Buffer|Uint8Array} data - Serialized data
   * @returns {Object} Deserialized packet
   */
  deserialize(data) {
    this.bytesReceived += data.length;
    const json = data.toString('utf8');
    return JSON.parse(json);
  }

  /**
   * Get protocol statistics
   * @returns {Object} Stats
   */
  getStats() {
    return {
      bytesSent: this.bytesSent,
      bytesReceived: this.bytesReceived,
      packetsLost: this.packetsLost,
      pendingAcks: this.pendingAcks.size,
      packetLossRate: this.packetsLost / this.nextSequenceId
    };
  }
}

// ============================================
// USAGE EXAMPLE
// ============================================

function demonstrateProtocol() {
  console.log('=== Network Protocol Demo ===\n');
  
  const clientProto = new NetworkProtocol();
  const serverProto = new NetworkProtocol();
  
  // Simulate network with packet loss
  function sendPacket(from, to, packet, lossRate = 0.1) {
    const serialized = from.serialize(packet);
    
    // Simulate packet loss
    if (Math.random() < lossRate) {
      console.log(`✗ Packet ${packet.sequenceId} lost in transit`);
      return null;
    }
    
    // Simulate network delay
    setTimeout(() => {
      const deserialized = to.deserialize(serialized);
      const result = to.processPacket(deserialized);
      
      if (result && result.ackPacket) {
        // Send ACK back
        setTimeout(() => {
          from.processAck(result.ackPacket.sequenceId);
        }, 10);
      }
      
      return result;
    }, 20 + Math.random() * 30);
  }
  
  // Client connects
  console.log('\n--- Client Connection ---');
  const helloPacket = clientProto.createClientHello('player_123', 'TestPlayer');
  console.log('Client → Server: HELLO');
  sendPacket(clientProto, serverProto, helloPacket);
  
  // Client sends inputs
  console.log('\n--- Sending Inputs ---');
  for (let tick = 0; tick < 3; tick++) {
    const inputPacket = clientProto.createInput(
      tick,
      { moveX: 1, moveY: 0, jump: false },
      Date.now()
    );
    console.log(`Client → Server: INPUT (tick ${tick})`);
    sendPacket(clientProto, serverProto, inputPacket, 0.2); // 20% loss
  }
  
  // Check for retransmissions
  setTimeout(() => {
    console.log('\n--- Checking for Retransmissions ---');
    const retransmit = clientProto.getRetransmissions();
    if (retransmit.length > 0) {
      console.log(`Retransmitting ${retransmit.length} packets`);
      retransmit.forEach(packet => {
        sendPacket(clientProto, serverProto, packet, 0); // No loss on retry
      });
    }
  }, 200);
  
  // Server sends snapshot
  setTimeout(() => {
    console.log('\n--- Server Snapshot ---');
    const snapshot = serverProto.createSnapshot(
      10,
      { entities: [{ id: 1, x: 100, y: 50 }] },
      false,
      2
    );
    console.log('Server → Client: SNAPSHOT');
    sendPacket(serverProto, clientProto, snapshot);
  }, 100);
  
  // Show stats
  setTimeout(() => {
    console.log('\n=== Protocol Statistics ===');
    console.log('Client:', clientProto.getStats());
    console.log('Server:', serverProto.getStats());
  }, 500);
}

// Run demo if executed directly
if (require.main === module) {
  demonstrateProtocol();
}

module.exports = { NetworkProtocol, MessageType };
