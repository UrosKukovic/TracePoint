"use client";

import React, { useEffect, useState, useRef } from 'react';
import * as signalR from "@microsoft/signalr";
import UplotReact from 'uplot-react';
import 'uplot/dist/uPlot.min.css';

export default function LiveDashboard() {
  const [isConnected, setIsConnected] = useState(false);
  const [lastValue, setLastValue] = useState(0);
  const [isRecording, setIsRecording] = useState(false);

  const connectionRef = useRef<signalR.HubConnection | null>(null);
  
  // uPlot potrebuje podatke v formatu [[x1, x2, ...], [y1, y2, ...]]
  const [chartData, setChartData] = useState<[number[], number[]]>([[], []]);
  
  // Ref-i so nujni za hitrost, da ne prožimo renderja ob vsakem bitu podatkov
  const xDataRef = useRef<number[]>([]);
  const yDataRef = useRef<number[]>([]);


  const options = {
    width: 800,
    height: 400,
    scales: {
      x: { time: true }, // Uporabljamo ms ali index
    },
    series: [
      {}, // Prva serija je vedno X os
      {
        label: "CAN Signal",
        stroke: "#3b82f6",
        width: 2,
        points: { show: false } // Točke upočasnjujejo, črta je dovolj
      },
    ],
    axes: [
      { stroke: "#64748b" },
      { stroke: "#64748b" }
    ],
  };

  useEffect(() => {
    const connection = new signalR.HubConnectionBuilder()
      .withUrl("http://localhost:5247/telemetryHub")
      .withAutomaticReconnect()
      .build();

    connectionRef.current = connection;

    connection.start().then(() => setIsConnected(true));

    connection.on("ReceiveMeasurement", (msg: any) => {
      setLastValue(msg.value);

      // 1. Dodaj nove podatke v ref (X in Y)
      xDataRef.current.push(msg.timestampMs / 1000);
      yDataRef.current.push(msg.value);

      // 2. KLJUČNO: Omeji dolžino polja (npr. na zadnjih 200 točk)
      const MAX_POINTS = 50; 
      if (xDataRef.current.length > MAX_POINTS) {
        xDataRef.current.shift(); // Odstrani prvo (najstarejšo) X vrednost
        yDataRef.current.shift(); // Odstrani prvo (najstarejšo) Y vrednost
      }

      // 3. Posodobi state z NOVIMI kopijami polj (React rabi nove reference, da ugotovi spremembo)
      setChartData([
        [...xDataRef.current], 
        [...yDataRef.current]
      ]);
    });

    return () => { connection.stop(); };
  }, []);

  const toggleRecording = async () => {
    if (!connectionRef.current) return;

    if (!isRecording) {
      const sessionName = `Test Run - ${new Date().toLocaleTimeString()}`;
      await connectionRef.current.invoke("StartRecording", sessionName);
      setIsRecording(true);
    } else {
      await connectionRef.current.invoke("StopRecording");
      setIsRecording(false);
    }
  };

  return (
    <div className="p-8 bg-slate-950 min-h-screen text-white">

      <div className="flex justify-between items-center mb-6">
        <h1 className="text-xl font-bold">TracePoint Live Stream</h1>
        
        {/* 3. Gumb za snemanje */}
        <button
          onClick={toggleRecording}
          disabled={!isConnected}
          className={`px-6 py-2 rounded-lg font-bold transition-all ${
            isRecording 
              ? "bg-red-600 hover:bg-red-700 animate-pulse" 
              : "bg-emerald-600 hover:bg-emerald-700 disabled:bg-slate-700"
          }`}
        >
          {isRecording ? "🔴 STOP RECORDING" : "⏺ START RECORDING"}
        </button>
      </div>

      <h1 className="text-xl mb-4">TracePoint High-Performance Stream</h1>
      
      <div className="bg-slate-900 p-6 rounded-xl border border-slate-800 inline-block mb-6">
        <p className="text-sm text-slate-400">Zadnja vrednost</p>
        <p className="text-4xl font-mono text-blue-400">{lastValue.toFixed(2)}</p>
      </div>

      <div className="bg-slate-900 p-4 rounded-xl border border-slate-800">
        <UplotReact
          options={options}
          data={chartData}
        />
      </div>
    </div>
  );
}