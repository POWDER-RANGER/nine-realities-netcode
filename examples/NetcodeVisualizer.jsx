import React, { useState, useEffect, useRef } from 'react';
import { Play, Pause, RotateCcw, Zap, Eye, Radio } from 'lucide-react';

const NetcodeVisualizer = () => {
  const [time, setTime] = useState(0);
  const [running, setRunning] = useState(false);
  const [latency, setLatency] = useState(50);
  const [packetLoss, setPacketLoss] = useState(0);
  const [mispredictAt, setMispredictAt] = useState(-1);
  const [showLagComp, setShowLagComp] = useState(false);
  
  const canvasRef = useRef(null);
  
  // Simulation state
  const [serverPos, setServerPos] = useState(100);
  const [clientPredPos, setClientPredPos] = useState(100);
  const [clientAuthPos, setClientAuthPos] = useState(100);
  const [packets, setPackets] = useState([]);
  const [inputs, setInputs] = useState([]);
  const [snapshots, setSnapshots] = useState([]);
  const [correcting, setCorrecting] = useState(false);
  const [hitMarker, setHitMarker] = useState(null);

  const tickRate = 64; // Server hz
  const clientRate = 144; // Client fps
  
  useEffect(() => {
    if (!running) return;
    
    const interval = setInterval(() => {
      setTime(t => t + 1);
      
      // Client generates input every frame
      if (time % Math.floor(clientRate / tickRate) === 0) {
        const input = {
          seq: Math.floor(time / (clientRate / tickRate)),
          time: time,
          move: Math.sin(time * 0.02) * 2, // Simulated movement
          sentAt: time,
          type: 'input'
        };
        setInputs(prev => [...prev, input]);
        
        // Send input packet (with latency)
        const packet = {
          ...input,
          arriveAt: time + latency,
          x: 150,
          y: 200,
          targetY: 350,
          lost: Math.random() < (packetLoss / 100)
        };
        setPackets(prev => [...prev, packet]);
        
        // Client prediction (instant)
        setClientPredPos(prev => {
          const newPos = prev + input.move;
          // Inject misprediction
          if (input.seq === mispredictAt) {
            return newPos + 30; // Wrong prediction
          }
          return newPos;
        });
      }
      
      // Server processes arrived inputs
      setPackets(prev => {
        const arrived = prev.filter(p => p.type === 'input' && p.arriveAt <= time && !p.lost);
        if (arrived.length > 0) {
          const latest = arrived[arrived.length - 1];
          setServerPos(prev => prev + latest.move);
          
          // Server sends snapshot back
          const snap = {
            seq: latest.seq,
            pos: serverPos + latest.move,
            ackSeq: latest.seq,
            time: time,
            sentAt: time,
            type: 'snapshot',
            arriveAt: time + latency,
            x: 350,
            y: 350,
            targetY: 200,
            lost: Math.random() < (packetLoss / 100)
          };
          setSnapshots(prev => [...prev, snap]);
          return [...prev.filter(p => p.arriveAt > time), snap];
        }
        return prev.filter(p => p.arriveAt > time);
      });
      
      // Client receives snapshots
      setSnapshots(prev => {
        const arrived = prev.filter(s => s.arriveAt <= time && !s.lost);
        if (arrived.length > 0) {
          const latest = arrived[arrived.length - 1];
          const delta = Math.abs(clientPredPos - latest.pos);
          
          // Check for misprediction
          if (delta > 5) {
            setCorrecting(true);
            setClientAuthPos(latest.pos);
            setTimeout(() => setCorrecting(false), 500);
          } else {
            setClientAuthPos(latest.pos);
          }
        }
        return prev.filter(s => s.arriveAt > time);
      });
      
    }, 1000 / clientRate);
    
    return () => clearInterval(interval);
  }, [running, time, latency, packetLoss, serverPos, clientPredPos, mispredictAt]);
  
  // Canvas rendering
  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    
    const ctx = canvas.getContext('2d');
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    
    // Draw client box
    ctx.fillStyle = '#1e293b';
    ctx.fillRect(50, 150, 200, 100);
    ctx.strokeStyle = '#3b82f6';
    ctx.lineWidth = 2;
    ctx.strokeRect(50, 150, 200, 100);
    ctx.fillStyle = '#fff';
    ctx.font = '14px monospace';
    ctx.fillText('CLIENT', 60, 175);
    ctx.font = '11px monospace';
    ctx.fillText(`Predicted: ${clientPredPos.toFixed(1)}`, 60, 195);
    ctx.fillText(`Auth: ${clientAuthPos.toFixed(1)}`, 60, 210);
    if (correcting) {
      ctx.fillStyle = '#ef4444';
      ctx.fillText('⚠ CORRECTING', 60, 230);
    }
    
    // Draw server box
    ctx.fillStyle = '#1e293b';
    ctx.fillRect(300, 300, 200, 100);
    ctx.strokeStyle = '#10b981';
    ctx.lineWidth = 2;
    ctx.strokeRect(300, 300, 200, 100);
    ctx.fillStyle = '#fff';
    ctx.font = '14px monospace';
    ctx.fillText('SERVER', 310, 325);
    ctx.font = '11px monospace';
    ctx.fillText(`Authority: ${serverPos.toFixed(1)}`, 310, 345);
    ctx.fillText(`Tick: ${Math.floor(time / (clientRate / tickRate))}`, 310, 360);
    
    // Draw packets in flight
    packets.forEach(p => {
      const progress = (time - p.sentAt) / (p.arriveAt - p.sentAt);
      const x = p.x;
      const y = p.y + (p.targetY - p.y) * progress;
      
      ctx.beginPath();
      ctx.arc(x, y, p.lost ? 5 : 6, 0, Math.PI * 2);
      ctx.fillStyle = p.lost ? '#ef4444' : (p.type === 'input' ? '#3b82f6' : '#10b981');
      ctx.fill();
      
      if (!p.lost) {
        ctx.fillStyle = '#fff';
        ctx.font = '9px monospace';
        ctx.fillText(p.seq, x + 8, y + 3);
      }
    });
    
    // Draw lag compensation visualization
    if (showLagComp) {
      ctx.fillStyle = 'rgba(251, 191, 36, 0.2)';
      ctx.fillRect(300, 450, 200, 80);
      ctx.strokeStyle = '#fbbf24';
      ctx.lineWidth = 2;
      ctx.strokeRect(300, 450, 200, 80);
      ctx.fillStyle = '#fff';
      ctx.font = '11px monospace';
      ctx.fillText('LAG COMP', 310, 470);
      ctx.fillText(`Rewind: -${latency}ms`, 310, 490);
      ctx.fillText('Hit: ' + (hitMarker ? '✓' : '○'), 310, 510);
    }
    
    // Draw timeline
    ctx.strokeStyle = '#64748b';
    ctx.beginPath();
    ctx.moveTo(50, 550);
    ctx.lineTo(500, 550);
    ctx.stroke();
    
    ctx.fillStyle = '#94a3b8';
    ctx.font = '10px monospace';
    ctx.fillText(`t=${time} | RTT=${latency*2}ms`, 50, 570);
    
  }, [time, packets, serverPos, clientPredPos, clientAuthPos, correcting, showLagComp, hitMarker, latency]);
  
  const reset = () => {
    setTime(0);
    setServerPos(100);
    setClientPredPos(100);
    setClientAuthPos(100);
    setPackets([]);
    setInputs([]);
    setSnapshots([]);
    setCorrecting(false);
    setMispredictAt(-1);
    setHitMarker(null);
  };
  
  const triggerMispredict = () => {
    setMispredictAt(Math.floor(time / (clientRate / tickRate)) + 5);
  };
  
  const triggerHit = () => {
    setShowLagComp(true);
    setHitMarker(time);
    setTimeout(() => {
      setHitMarker(null);
      setShowLagComp(false);
    }, 1000);
  };

  return (
    <div className="w-full h-screen bg-slate-900 text-white p-6 overflow-auto">
      <div className="max-w-6xl mx-auto">
        <h1 className="text-3xl font-bold mb-2 flex items-center gap-3">
          <Radio className="text-blue-400" />
          Nine Realities Netcode
        </h1>
        <p className="text-slate-400 mb-6 text-sm">
          Interactive visualization: Client prediction, server authority, reconciliation
        </p>
        
        <div className="grid grid-cols-1 lg:grid-cols-3 gap-6 mb-6">
          {/* Controls */}
          <div className="bg-slate-800 rounded-lg p-4">
            <h3 className="text-lg font-semibold mb-4 flex items-center gap-2">
              <Zap className="w-5 h-5 text-yellow-400" />
              Controls
            </h3>
            
            <div className="space-y-4">
              <div className="flex gap-2">
                <button
                  onClick={() => setRunning(!running)}
                  className="flex-1 bg-blue-600 hover:bg-blue-700 px-4 py-2 rounded flex items-center justify-center gap-2"
                >
                  {running ? <Pause className="w-4 h-4" /> : <Play className="w-4 h-4" />}
                  {running ? 'Pause' : 'Start'}
                </button>
                <button
                  onClick={reset}
                  className="bg-slate-700 hover:bg-slate-600 px-4 py-2 rounded"
                >
                  <RotateCcw className="w-4 h-4" />
                </button>
              </div>
              
              <div>
                <label className="block text-sm mb-2">
                  Latency: {latency}ms (RTT: {latency*2}ms)
                </label>
                <input
                  type="range"
                  min="10"
                  max="200"
                  value={latency}
                  onChange={(e) => setLatency(Number(e.target.value))}
                  className="w-full"
                />
              </div>
              
              <div>
                <label className="block text-sm mb-2">
                  Packet Loss: {packetLoss}%
                </label>
                <input
                  type="range"
                  min="0"
                  max="30"
                  value={packetLoss}
                  onChange={(e) => setPacketLoss(Number(e.target.value))}
                  className="w-full"
                />
              </div>
              
              <button
                onClick={triggerMispredict}
                disabled={!running}
                className="w-full bg-red-600 hover:bg-red-700 disabled:bg-slate-700 px-4 py-2 rounded text-sm"
              >
                Inject Misprediction
              </button>
              
              <button
                onClick={triggerHit}
                disabled={!running}
                className="w-full bg-yellow-600 hover:bg-yellow-700 disabled:bg-slate-700 px-4 py-2 rounded text-sm"
              >
                Simulate Hit (Lag Comp)
              </button>
            </div>
          </div>
          
          {/* Legend */}
          <div className="bg-slate-800 rounded-lg p-4">
            <h3 className="text-lg font-semibold mb-4 flex items-center gap-2">
              <Eye className="w-5 h-5 text-green-400" />
              Legend
            </h3>
            
            <div className="space-y-3 text-sm">
              <div className="flex items-center gap-3">
                <div className="w-4 h-4 rounded-full bg-blue-500"></div>
                <span>INPUT packets (client → server)</span>
              </div>
              <div className="flex items-center gap-3">
                <div className="w-4 h-4 rounded-full bg-green-500"></div>
                <span>SNAPSHOT packets (server → client)</span>
              </div>
              <div className="flex items-center gap-3">
                <div className="w-4 h-4 rounded-full bg-red-500"></div>
                <span>Lost packet</span>
              </div>
              <div className="flex items-center gap-3">
                <div className="w-3 h-3 bg-blue-400 border-2 border-blue-400"></div>
                <span>Client predicted state</span>
              </div>
              <div className="flex items-center gap-3">
                <div className="w-3 h-3 bg-green-400 border-2 border-green-400"></div>
                <span>Server authoritative state</span>
              </div>
              <div className="flex items-center gap-3">
                <span className="text-red-400">⚠</span>
                <span>Rollback + replay correction</span>
              </div>
            </div>
          </div>
          
          {/* Stats */}
          <div className="bg-slate-800 rounded-lg p-4">
            <h3 className="text-lg font-semibold mb-4">Live Stats</h3>
            
            <div className="space-y-2 text-sm font-mono">
              <div className="flex justify-between">
                <span className="text-slate-400">Server Tick:</span>
                <span>{Math.floor(time / (clientRate / tickRate))}</span>
              </div>
              <div className="flex justify-between">
                <span className="text-slate-400">Client Frame:</span>
                <span>{time}</span>
              </div>
              <div className="flex justify-between">
                <span className="text-slate-400">Packets In-Flight:</span>
                <span>{packets.length}</span>
              </div>
              <div className="flex justify-between">
                <span className="text-slate-400">Input Buffer:</span>
                <span>{inputs.length} inputs</span>
              </div>
              <div className="flex justify-between">
                <span className="text-slate-400">Prediction Error:</span>
                <span className={Math.abs(clientPredPos - serverPos) > 5 ? 'text-red-400' : 'text-green-400'}>
                  Δ{Math.abs(clientPredPos - serverPos).toFixed(1)}
                </span>
              </div>
              <div className="flex justify-between">
                <span className="text-slate-400">Status:</span>
                <span className={correcting ? 'text-red-400' : 'text-green-400'}>
                  {correcting ? 'CORRECTING' : 'SYNCED'}
                </span>
              </div>
            </div>
          </div>
        </div>
        
        {/* Canvas */}
        <div className="bg-slate-800 rounded-lg p-4">
          <canvas
            ref={canvasRef}
            width={550}
            height={580}
            className="w-full border border-slate-700 rounded"
          />
        </div>
        
        {/* Technical Notes */}
        <div className="mt-6 bg-slate-800 rounded-lg p-4">
          <h3 className="text-lg font-semibold mb-3">What You're Seeing</h3>
          <div className="grid grid-cols-1 md:grid-cols-2 gap-4 text-sm text-slate-300">
            <div>
              <h4 className="text-white font-semibold mb-2">Client Prediction</h4>
              <p>Client runs simulation instantly without waiting for server. Blue position updates every frame ({clientRate}fps).</p>
            </div>
            <div>
              <h4 className="text-white font-semibold mb-2">Server Authority</h4>
              <p>Server receives inputs with {latency}ms delay, validates, simulates at {tickRate}hz, and sends back authoritative snapshots.</p>
            </div>
            <div>
              <h4 className="text-white font-semibold mb-2">Reconciliation</h4>
              <p>When client's prediction diverges from server snapshot (Δ &gt; 5), client rolls back to server state and replays buffered inputs.</p>
            </div>
            <div>
              <h4 className="text-white font-semibold mb-2">Lag Compensation</h4>
              <p>Server rewinds entity history by RTT/2 to validate hits at the time client saw the target, compensating for network delay.</p>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
};

export default NetcodeVisualizer;
