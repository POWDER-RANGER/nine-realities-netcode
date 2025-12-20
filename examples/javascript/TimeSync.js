/**
 * Time Synchronization System for Nine Realities Netcode
 * 
 * Provides clock synchronization between client and server using:
 * - Network Time Protocol (NTP)-like algorithm
 * - Smoothed RTT (Round-Trip Time) estimation
 * - Clock offset calculation with drift compensation
 * - Tick-based simulation timestep conversion
 * 
 * Critical for reconciliation: clients must map local inputs to exact server ticks.
 */

class TimeSync {
  constructor(config = {}) {
    // Configuration
    this.tickRate = config.tickRate || 60; // Server ticks per second
    this.tickDuration = 1000 / this.tickRate; // ms per tick
    this.syncInterval = config.syncInterval || 1000; // How often to sync (ms)
    this.rttSamples = config.rttSamples || 10; // Samples for RTT smoothing
    
    // State
    this.serverTick = 0; // Last known server tick
    this.clockOffset = 0; // Client clock - Server clock (ms)
    this.rttHistory = []; // Recent RTT measurements
    this.smoothedRtt = 0; // Exponentially weighted RTT
    this.clockDrift = 0; // ms/s clock drift rate
    
    // Timestamps
    this.lastSyncTime = 0;
    this.lastDriftCheckTime = Date.now();
    this.lastDriftOffset = 0;
    
    // Statistics
    this.syncCount = 0;
    this.jitter = 0; // RTT variance
  }

  /**
   * Client: Send time sync request to server
   * @returns {Object} Sync request packet
   */
  createSyncRequest() {
    const clientTimestamp = this.getLocalTime();
    return {
      type: 'time_sync_req',
      clientSendTime: clientTimestamp,
      sequenceId: this.syncCount++
    };
  }

  /**
   * Server: Process sync request and create response
   * @param {Object} request - Client's sync request
   * @param {number} serverTick - Current server tick
   * @returns {Object} Sync response packet
   */
  handleSyncRequest(request, serverTick) {
    const serverTime = this.getLocalTime();
    return {
      type: 'time_sync_res',
      clientSendTime: request.clientSendTime,
      serverReceiveTime: serverTime,
      serverSendTime: serverTime, // Can add processing delay if needed
      serverTick: serverTick,
      sequenceId: request.sequenceId
    };
  }

  /**
   * Client: Process sync response from server
   * @param {Object} response - Server's sync response
   */
  processSyncResponse(response) {
    const clientReceiveTime = this.getLocalTime();
    
    // Calculate RTT: total round-trip time
    const rtt = clientReceiveTime - response.clientSendTime;
    
    // Calculate one-way latency (assume symmetric)
    const oneWayLatency = rtt / 2;
    
    // Estimate server time when we received the response
    const estimatedServerTime = response.serverSendTime + oneWayLatency;
    
    // Calculate clock offset
    const newOffset = clientReceiveTime - estimatedServerTime;
    
    // Update RTT statistics
    this.updateRttStats(rtt);
    
    // Update clock offset with smoothing
    this.updateClockOffset(newOffset);
    
    // Update server tick
    this.serverTick = response.serverTick;
    this.lastSyncTime = clientReceiveTime;
    
    // Update drift estimation periodically
    this.updateClockDrift(newOffset);
    
    console.log(`🕐 Time sync: RTT=${rtt.toFixed(1)}ms, Offset=${this.clockOffset.toFixed(1)}ms, Drift=${this.clockDrift.toFixed(3)}ms/s`);
  }

  /**
   * Update RTT statistics with exponential smoothing
   * @param {number} rtt - New RTT measurement
   */
  updateRttStats(rtt) {
    // Add to history
    this.rttHistory.push(rtt);
    if (this.rttHistory.length > this.rttSamples) {
      this.rttHistory.shift();
    }
    
    // Exponential weighted moving average
    const alpha = 0.125; // Smoothing factor (typical for TCP RTT)
    if (this.smoothedRtt === 0) {
      this.smoothedRtt = rtt;
    } else {
      this.smoothedRtt = alpha * rtt + (1 - alpha) * this.smoothedRtt;
    }
    
    // Calculate jitter (mean deviation)
    const meanRtt = this.rttHistory.reduce((a, b) => a + b, 0) / this.rttHistory.length;
    const variance = this.rttHistory.reduce((sum, val) => sum + Math.abs(val - meanRtt), 0) / this.rttHistory.length;
    this.jitter = variance;
  }

  /**
   * Update clock offset with smoothing to prevent jitter
   * @param {number} newOffset - New offset measurement
   */
  updateClockOffset(newOffset) {
    if (this.clockOffset === 0) {
      // First measurement: accept immediately
      this.clockOffset = newOffset;
    } else {
      // Smooth updates to prevent visual jitter
      const delta = newOffset - this.clockOffset;
      
      // If offset change is large, accept more quickly
      const threshold = 50; // ms
      const blendFactor = Math.abs(delta) > threshold ? 0.5 : 0.1;
      
      this.clockOffset += delta * blendFactor;
    }
  }

  /**
   * Update clock drift estimation
   * @param {number} currentOffset - Current offset measurement
   */
  updateClockDrift(currentOffset) {
    const now = Date.now();
    const elapsed = (now - this.lastDriftCheckTime) / 1000; // seconds
    
    // Only update drift estimate after sufficient time has passed
    if (elapsed > 10) {
      const offsetDelta = currentOffset - this.lastDriftOffset;
      this.clockDrift = offsetDelta / elapsed; // ms per second
      
      this.lastDriftCheckTime = now;
      this.lastDriftOffset = currentOffset;
    }
  }

  /**
   * Get current local timestamp
   * @returns {number} Timestamp in milliseconds
   */
  getLocalTime() {
    return Date.now();
  }

  /**
   * Get estimated server time
   * @returns {number} Server time in milliseconds
   */
  getServerTime() {
    const localTime = this.getLocalTime();
    const timeSinceSync = localTime - this.lastSyncTime;
    
    // Apply clock offset and drift correction
    const driftCorrection = this.clockDrift * (timeSinceSync / 1000);
    return localTime - this.clockOffset - driftCorrection;
  }

  /**
   * Get estimated current server tick
   * @returns {number} Server tick
   */
  getServerTick() {
    const serverTime = this.getServerTime();
    const timeSinceSync = serverTime - (this.lastSyncTime - this.clockOffset);
    const ticksSinceSync = Math.floor(timeSinceSync / this.tickDuration);
    return this.serverTick + ticksSinceSync;
  }

  /**
   * Convert local timestamp to server tick
   * @param {number} localTimestamp - Local timestamp
   * @returns {number} Corresponding server tick
   */
  localTimeToServerTick(localTimestamp) {
    const serverTime = localTimestamp - this.clockOffset;
    return Math.floor(serverTime / this.tickDuration);
  }

  /**
   * Convert server tick to estimated server time
   * @param {number} tick - Server tick
   * @returns {number} Server time in milliseconds
   */
  serverTickToTime(tick) {
    return tick * this.tickDuration;
  }

  /**
   * Get synchronization quality metrics
   * @returns {Object} Sync quality stats
   */
  getSyncQuality() {
    return {
      smoothedRtt: this.smoothedRtt,
      jitter: this.jitter,
      clockOffset: this.clockOffset,
      clockDrift: this.clockDrift,
      quality: this.calculateQuality()
    };
  }

  /**
   * Calculate overall sync quality score (0-1)
   * @returns {number} Quality score
   */
  calculateQuality() {
    // Perfect: RTT < 50ms, jitter < 10ms
    // Good: RTT < 100ms, jitter < 20ms
    // Fair: RTT < 200ms, jitter < 50ms
    // Poor: RTT > 200ms or jitter > 50ms
    
    let score = 1.0;
    
    // RTT penalty
    if (this.smoothedRtt > 200) score *= 0.3;
    else if (this.smoothedRtt > 100) score *= 0.6;
    else if (this.smoothedRtt > 50) score *= 0.8;
    
    // Jitter penalty
    if (this.jitter > 50) score *= 0.3;
    else if (this.jitter > 20) score *= 0.6;
    else if (this.jitter > 10) score *= 0.8;
    
    return score;
  }

  /**
   * Check if synchronization is reliable
   * @returns {boolean} True if sync is good
   */
  isSyncReliable() {
    return this.calculateQuality() > 0.5 && this.syncCount > 3;
  }
}

// ============================================
// USAGE EXAMPLE
// ============================================

function demonstrateTimeSync() {
  console.log('=== Time Synchronization Demo ===\n');
  
  // Create client and server instances
  const clientSync = new TimeSync({ tickRate: 60 });
  const serverSync = new TimeSync({ tickRate: 60 });
  
  // Simulate network with variable latency
  function simulateNetwork(packet, callback, latencyMs = 50) {
    setTimeout(() => callback(packet), latencyMs + Math.random() * 20);
  }
  
  let serverTickCounter = 0;
  
  // Perform sync cycle
  function performSync() {
    console.log(`\n--- Sync #${clientSync.syncCount + 1} ---`);
    
    // Client: Create sync request
    const request = clientSync.createSyncRequest();
    console.log('Client → Server: Sync request sent');
    
    // Simulate network delay to server
    simulateNetwork(request, (req) => {
      // Server: Handle request and create response
      serverTickCounter += 3; // Simulate server advancing
      const response = serverSync.handleSyncRequest(req, serverTickCounter);
      console.log(`Server: Processing sync (tick ${serverTickCounter})`);
      
      // Simulate network delay back to client
      simulateNetwork(response, (res) => {
        // Client: Process response
        clientSync.processSyncResponse(res);
        
        // Show results
        const quality = clientSync.getSyncQuality();
        console.log(`Client estimated server tick: ${clientSync.getServerTick()}`);
        console.log(`Sync quality: ${(quality.quality * 100).toFixed(0)}%`);
      }, 50);
    }, 50);
  }
  
  // Perform multiple syncs
  performSync();
  setTimeout(() => performSync(), 500);
  setTimeout(() => performSync(), 1000);
  setTimeout(() => performSync(), 1500);
  
  // Show final stats
  setTimeout(() => {
    console.log('\n=== Final Statistics ===');
    const quality = clientSync.getSyncQuality();
    console.log(`Smoothed RTT: ${quality.smoothedRtt.toFixed(1)}ms`);
    console.log(`Jitter: ${quality.jitter.toFixed(1)}ms`);
    console.log(`Clock offset: ${quality.clockOffset.toFixed(1)}ms`);
    console.log(`Sync quality: ${(quality.quality * 100).toFixed(0)}%`);
    console.log(`Is reliable: ${clientSync.isSyncReliable()}`);
  }, 2000);
}

// Run demo if executed directly
if (require.main === module) {
  demonstrateTimeSync();
}

module.exports = TimeSync;
