"use client";

import React, { useEffect, useState, useRef } from 'react';
import * as signalR from "@microsoft/signalr";
import { LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip, ResponsiveContainer } from 'recharts';
import { Activity, Cpu, Database } from 'lucide-react';

// Tip za podatke (mora se ujemati z CanMeasurementDto)
interface CanMeasurement {
  timestampMs: number;
  canId: number;
  value: number;
  channel: string;
}

export default function LiveDashboard() {
  const [data, setData] = useState<CanMeasurement[]>([]);
  const [lastMessage, setLastMessage] = useState<CanMeasurement | null>(null);
  const [isConnected, setIsConnected] = useState(false);
  
  // Uporabimo ref, da imamo vedno zadnje podatke v SignalR callbacku
  const dataRef = useRef<CanMeasurement[]>([]);

  useEffect(() => {
    const connection = new signalR.HubConnectionBuilder()
      .withUrl("http://localhost:5247/telemetryHub") // PREVERI PORT!
      .withAutomaticReconnect()
      .build();

    connection.start()
      .then(() => {
        setIsConnected(true);
        console.log("Povezan na SignalR!");
      })
      .catch(err => console.error("SignalR Error: ", err));

    connection.on("ReceiveMeasurement", (message: CanMeasurement) => {
      setLastMessage(message);
      
      // Dodaj v seznam in obdrži zadnjih 50 točk
      const newData = [...dataRef.current, { ...message, displayTime: new Date().toLocaleTimeString() }];
      const trimmedData = newData.slice(-50);
      
      dataRef.current = trimmedData;
      setData(trimmedData);
    });

    return () => {
      connection.stop();
    };
  }, []);

  return (
    <div className="min-h-screen bg-slate-950 text-slate-100 p-8">
      <header className="flex justify-between items-center mb-8 border-b border-slate-800 pb-4">
        <div>
          <h1 className="text-2xl font-bold bg-gradient-to-r from-blue-400 to-emerald-400 bg-clip-text text-transparent">
            TracePoint Pulse
          </h1>
          <p className="text-slate-500 text-sm">Real-time Hardware Telemetry</p>
        </div>
        <div className={`flex items-center gap-2 px-3 py-1 rounded-full text-xs font-medium ${isConnected ? 'bg-emerald-500/10 text-emerald-400' : 'bg-red-500/10 text-red-400'}`}>
          <div className={`w-2 h-2 rounded-full ${isConnected ? 'bg-emerald-500 animate-pulse' : 'bg-red-500'}`} />
          {isConnected ? 'LIVE' : 'DISCONNECTED'}
        </div>
      </header>

      <div className="grid grid-cols-1 lg:grid-cols-3 gap-6">
        {/* Zadnja vrednost Card */}
        <div className="bg-slate-900 border border-slate-800 p-6 rounded-2xl shadow-xl">
          <div className="flex items-center gap-3 mb-4 text-slate-400">
            <Activity size={20} />
            <h2 className="font-semibold">Current Value</h2>
          </div>
          <div className="text-5xl font-mono font-bold text-blue-400 mb-2">
            {lastMessage?.value.toFixed(2) ?? "0.00"}
          </div>
          <div className="space-y-2 mt-4 text-sm text-slate-400 border-t border-slate-800 pt-4">
            <div className="flex justify-between">
              <span>CAN ID:</span>
              <span className="text-slate-100 font-mono">0x{lastMessage?.canId.toString(16).toUpperCase() ?? "---"}</span>
            </div>
            <div className="flex justify-between">
              <span>Channel:</span>
              <span className="text-slate-100">{lastMessage?.channel ?? "Searching..."}</span>
            </div>
          </div>
        </div>

        {/* Graf Card */}
        <div className="lg:col-span-2 bg-slate-900 border border-slate-800 p-6 rounded-2xl shadow-xl h-[400px]">
          <div className="flex items-center gap-3 mb-6 text-slate-400">
            <Cpu size={20} />
            <h2 className="font-semibold">Live Signal</h2>
          </div>
          <ResponsiveContainer width="100%" height="100%">
            <LineChart data={data}>
              <CartesianGrid strokeDasharray="3 3" stroke="#1e293b" />
              <XAxis dataKey="displayTime" stroke="#64748b" fontSize={12} tickCount={5} />
              <YAxis stroke="#64748b" fontSize={12} domain={[0, 100]} />
              <Tooltip 
                contentStyle={{ backgroundColor: '#0f172a', border: '1px solid #1e293b' }}
                itemStyle={{ color: '#60a5fa' }}
              />
              <Line 
                type="monotone" 
                dataKey="value" 
                stroke="#3b82f6" 
                strokeWidth={3} 
                dot={false} 
                animationDuration={0} // Nujno za real-time!
              />
            </LineChart>
          </ResponsiveContainer>
        </div>
      </div>
    </div>
  );
}