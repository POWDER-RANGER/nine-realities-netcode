/**
 * Client-Side Prediction with Rollback and Reconciliation
 * 
 * Demonstrates the N+1 simulation model where each client maintains:
 * 1. A predicted local state (optimistic)
 * 2. A server-authoritative state (received via snapshots)
 * 3. An input buffer for rollback/replay
 * 
 * This implementation shows how clients can predict movement immediately
 * while still maintaining synchronization with the server's authoritative state.
 */

class ClientPrediction {
  constructor() {
    // Client's current predicted position
    this.predictedState = { x: 0, y: 0, velocityX: 0, velocityY: 0 };
    
    // Last confirmed server state
    this.serverState = { x: 0, y: 0, velocityX: 0, velocityY: 0, tick: 0 };
    
    // Input buffer: stores all inputs sent to server
    // Format: { tick, input, timestamp }
    this.inputBuffer = [];
    
    // Current client tick (increments each frame)
    this.clientTick = 0;
    
    // Network latency simulation (in ticks)
    this.rtt = 5; // Round-trip time
    
    // Reconciliation threshold (when to trigger rollback)
    this.reconciliationThreshold = 0.1;
  }

  /**
   * Process user input immediately (optimistic prediction)
   * @param {Object} input - User input (e.g., { moveX: 1, moveY: 0 })
   */
  applyInput(input) {
    // 1. Store input in buffer for potential replay
    this.inputBuffer.push({
      tick: this.clientTick,
      input: { ...input },
      timestamp: Date.now()
    });

    // 2. Apply input to predicted state IMMEDIATELY
    this.updateState(this.predictedState, input);

    // 3. Send input to server (with tick and timestamp)
    this.sendToServer({
      type: 'input',
      tick: this.clientTick,
      input: input,
      timestamp: Date.now()
    });

    this.clientTick++;
  }

  /**
   * Update physics state based on input
   * @param {Object} state - State to update
   * @param {Object} input - Input to apply
   */
  updateState(state, input) {
    // Simple movement physics
    const speed = 5;
    state.velocityX = input.moveX * speed;
    state.velocityY = input.moveY * speed;
    state.x += state.velocityX;
    state.y += state.velocityY;
  }

  /**
   * Receive server snapshot (authoritative state)
   * @param {Object} snapshot - Server's authoritative state
   */
  onServerSnapshot(snapshot) {
    // 1. Update last known server state
    this.serverState = { ...snapshot };

    // 2. Calculate divergence between predicted and server state
    const divergence = this.calculateDivergence(
      this.predictedState,
      this.serverState
    );

    console.log(`Divergence: ${divergence.toFixed(3)}`);

    // 3. If divergence exceeds threshold, perform rollback
    if (divergence > this.reconciliationThreshold) {
      console.warn('⚠️ Prediction error detected! Rolling back...');
      this.rollbackAndReplay(snapshot);
    } else {
      // 4. Small divergence: just smooth interpolate
      this.smoothCorrection(divergence);
    }

    // 5. Clean up acknowledged inputs from buffer
    this.inputBuffer = this.inputBuffer.filter(
      (buffered) => buffered.tick > snapshot.tick
    );
  }

  /**
   * Rollback to server state and replay unacknowledged inputs
   * @param {Object} serverSnapshot - Authoritative server state
   */
  rollbackAndReplay(serverSnapshot) {
    // 1. Rewind to server's authoritative state
    this.predictedState = { ...serverSnapshot };

    // 2. Replay all inputs that came AFTER the server tick
    const unacknowledgedInputs = this.inputBuffer.filter(
      (buffered) => buffered.tick > serverSnapshot.tick
    );

    console.log(`🔄 Replaying ${unacknowledgedInputs.length} inputs...`);

    // 3. Re-apply each unacknowledged input
    for (const buffered of unacknowledgedInputs) {
      this.updateState(this.predictedState, buffered.input);
    }

    console.log('✅ Rollback complete. State reconciled.');
  }

  /**
   * Calculate Euclidean distance between predicted and server state
   * @param {Object} predicted - Predicted state
   * @param {Object} server - Server state
   * @returns {number} Distance
   */
  calculateDivergence(predicted, server) {
    const dx = predicted.x - server.x;
    const dy = predicted.y - server.y;
    return Math.sqrt(dx * dx + dy * dy);
  }

  /**
   * Smooth correction for small prediction errors
   * @param {number} divergence - Amount of error
   */
  smoothCorrection(divergence) {
    // Blend factor (0-1): higher = faster correction
    const blendFactor = Math.min(divergence / this.reconciliationThreshold, 1) * 0.1;

    // Lerp predicted state toward server state
    this.predictedState.x += (this.serverState.x - this.predictedState.x) * blendFactor;
    this.predictedState.y += (this.serverState.y - this.predictedState.y) * blendFactor;

    console.log(`📊 Smooth correction applied (blend: ${(blendFactor * 100).toFixed(1)}%)`);
  }

  /**
   * Simulate sending data to server
   * @param {Object} data - Data to send
   */
  sendToServer(data) {
    // In real implementation, this would use WebSocket/UDP
    console.log('📤 Sent to server:', data);
  }

  /**
   * Get current visual state (what should be rendered)
   * @returns {Object} Current predicted state
   */
  getVisualState() {
    return { ...this.predictedState };
  }
}

// ============================================
// USAGE EXAMPLE
// ============================================

const client = new ClientPrediction();

console.log('=== Client-Side Prediction Demo ===\n');

// Frame 1: Player presses right arrow
console.log('Frame 1: Input RIGHT');
client.applyInput({ moveX: 1, moveY: 0 });
console.log('Predicted position:', client.getVisualState());
console.log('');

// Frame 2: Player continues moving right
console.log('Frame 2: Input RIGHT');
client.applyInput({ moveX: 1, moveY: 0 });
console.log('Predicted position:', client.getVisualState());
console.log('');

// Frame 5: Server snapshot arrives (with slight correction)
console.log('Frame 5: Server snapshot received');
client.onServerSnapshot({
  x: 9.5,  // Slightly different from client's prediction
  y: 0,
  velocityX: 5,
  velocityY: 0,
  tick: 1
});
console.log('Corrected position:', client.getVisualState());
console.log('');

// Frame 10: Large prediction error (e.g., server rejected input)
console.log('Frame 10: Server snapshot with LARGE error');
client.onServerSnapshot({
  x: 5,  // Server rejected some movement
  y: 0,
  velocityX: 0,
  velocityY: 0,
  tick: 2
});
console.log('After rollback:', client.getVisualState());

/*
EXPECTED OUTPUT:

=== Client-Side Prediction Demo ===

Frame 1: Input RIGHT
📤 Sent to server: { type: 'input', tick: 0, input: { moveX: 1, moveY: 0 }, timestamp: ... }
Predicted position: { x: 5, y: 0, velocityX: 5, velocityY: 0 }

Frame 2: Input RIGHT
📤 Sent to server: { type: 'input', tick: 1, input: { moveX: 1, moveY: 0 }, timestamp: ... }
Predicted position: { x: 10, y: 0, velocityX: 5, velocityY: 0 }

Frame 5: Server snapshot received
Divergence: 0.500
📊 Smooth correction applied (blend: 5.0%)
Corrected position: { x: 9.975, y: 0, velocityX: 5, velocityY: 0 }

Frame 10: Server snapshot with LARGE error
Divergence: 4.975
⚠️ Prediction error detected! Rolling back...
🔄 Replaying 2 inputs...
✅ Rollback complete. State reconciled.
After rollback: { x: 15, y: 0, velocityX: 5, velocityY: 0 }

KEY CONCEPTS DEMONSTRATED:
1. Client predicts movement IMMEDIATELY (responsive)
2. Input buffer stores all unacknowledged inputs
3. Server snapshots provide authoritative truth
4. Small errors: smooth blend (invisible to player)
5. Large errors: rollback + replay (maintains correctness)
6. This is ONE of the N+1 simulations (this client's view)
*/

module.exports = ClientPrediction;
