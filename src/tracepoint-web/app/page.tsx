"use client";

import React, { useEffect, useState, useRef } from 'react';
import * as signalR from "@microsoft/signalr";
import UplotReact from 'uplot-react';
import 'uplot/dist/uPlot.min.css';
import { useRouter } from 'next/navigation';

// 1. Singleton connection outside the component lifecycle
let sharedConnection: signalR.HubConnection | null = null;

export default function LiveDashboard() {
  const router = useRouter();
  const [isConnected, setIsConnected] = useState(false);
  const [lastValue, setLastValue] = useState(0);
  const [isRecording, setIsRecording] = useState(false);
  const [sessions, setSessions] = useState<any[]>([]);
  
  // uPlot data state
  const [chartData, setChartData] = useState<[number[], number[]]>([[], []]);
  
  // Performance Refs
  const xDataRef = useRef<number[]>([]);
  const yDataRef = useRef<number[]>([]);
  const globalTimeOffsetRef = useRef<number | null>(null);

  // 2. Define options inside the component
  const options = {
    width: 800,
    height: 400,
    scales: { x: { time: true } },
    series: [{}, {
      label: "CAN Signal",
      stroke: "#3b82f6",
      width: 2,
      points: { show: false }
    }],
    axes: [{ stroke: "#64748b" }, { stroke: "#64748b" }],
  };

  // 3. Define loadSessions function
  const loadSessions = async () => {
    try {
      const r = await fetch("http://localhost:5247/api/telemetry/sessions");
      const data = await r.json();
      setSessions(data);
    } catch (e) {
      console.error("Napaka pri branju sej", e);
    }
  };

  useEffect(() => {
    // Initialize connection if it doesn't exist
    if (!sharedConnection) {
      sharedConnection = new signalR.HubConnectionBuilder()
        .withUrl("http://localhost:5247/telemetryHub")
        .withAutomaticReconnect()
        .build();
    }

    const startConnection = async () => {
      if (sharedConnection!.state === signalR.HubConnectionState.Disconnected) {
        try {
          await sharedConnection!.start();
          setIsConnected(true);
          console.log("SignalR Connected");
        } catch (err) {
          console.error("SignalR Start Error: ", err);
        }
      } else if (sharedConnection!.state === signalR.HubConnectionState.Connected) {
        setIsConnected(true);
      }
    };

    startConnection();
    loadSessions();

    // Attach listener
    sharedConnection.on("ReceiveMeasurement", (msg: any) => {
      setLastValue(msg.value);

      if (globalTimeOffsetRef.current === null) {
          globalTimeOffsetRef.current = Date.now() - msg.timestampMs;
      }

      const displayTime = (msg.timestampMs + globalTimeOffsetRef.current) / 1000;

      xDataRef.current.push(displayTime);
      yDataRef.current.push(msg.value);

      const MAX_POINTS = 200; 
      if (xDataRef.current.length > MAX_POINTS) {
        xDataRef.current.shift();
        yDataRef.current.shift();
      }

      setChartData([[...xDataRef.current], [...yDataRef.current]]);
    });

    // Clean up only the listener, keep connection alive
    return () => {
      if (sharedConnection) {
        sharedConnection.off("ReceiveMeasurement");
      }
    };
  }, []);

  const toggleRecording = async () => {
    if (!sharedConnection || sharedConnection.state !== signalR.HubConnectionState.Connected) return;
    
    if (!isRecording) {
      const sessionName = `Test Run - ${new Date().toLocaleTimeString()}`;
      await sharedConnection.invoke("StartRecording", sessionName);
      setIsRecording(true);
    } else {
      await sharedConnection.invoke("StopRecording");
      setIsRecording(false);
      setTimeout(loadSessions, 500); // Reload history after stop
    }
  };

  return (
    <div className="p-8 bg-slate-950 min-h-screen text-white">
      <div className="flex justify-between items-center mb-6">
        <h1 className="text-2xl font-bold bg-gradient-to-r from-blue-400 to-emerald-400 bg-clip-text text-transparent">
          TracePoint Live Dashboard
        </h1>
        
        <button
          onClick={toggleRecording}
          disabled={!isConnected}
          className={`px-6 py-2 rounded-lg font-bold transition-all shadow-lg ${
            isRecording 
              ? "bg-red-600 hover:bg-red-700 animate-pulse shadow-red-900/20" 
              : "bg-emerald-600 hover:bg-emerald-700 disabled:bg-slate-700 shadow-emerald-900/20"
          }`}
        >
          {isRecording ? "🔴 RECORDING..." : "⏺ START RECORDING"}
        </button>
      </div>

      <div className="grid grid-cols-1 md:grid-cols-3 gap-6 mb-8">
        <div className="bg-slate-900 p-6 rounded-xl border border-slate-800 shadow-xl">
          <p className="text-sm text-slate-400 uppercase tracking-wider font-semibold">Live CAN Value</p>
          <p className="text-5xl font-mono text-blue-400 mt-2">{lastValue.toFixed(2)}</p>
        </div>
        
        <div className="md:col-span-2 bg-slate-900 p-4 rounded-xl border border-slate-800 shadow-xl overflow-hidden">
          <UplotReact options={options} data={chartData} />
        </div>
      </div>

      <div className="mt-10">
        <h2 className="text-xl font-bold mb-6 text-slate-300 flex items-center gap-2">
          <span className="w-2 h-2 bg-blue-500 rounded-full"></span>
          Session History
        </h2>
        <div className="grid grid-cols-1 gap-3">
          {sessions.map(s => (
            <div key={s.id} className="group p-4 bg-slate-900/50 hover:bg-slate-900 rounded-xl border border-slate-800 flex justify-between items-center transition-all">
              <div>
                <p className="font-bold text-slate-200 group-hover:text-blue-400 transition-colors">{s.name}</p>
                <p className="text-slate-500 text-xs">{new Date(s.createdAt).toLocaleString()}</p>
              </div>
              <button 
                onClick={() => router.push(`/analytics/${s.id}`)}
                className="bg-slate-800 hover:bg-blue-600 px-5 py-2 rounded-lg text-sm font-bold transition-all"
              >
                ANALYZE DATA →
              </button>
            </div>
          ))}
        </div>
      </div>
    </div>
  );
}