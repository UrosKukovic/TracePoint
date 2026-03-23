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

  const sessionStartRealRef = useRef<number | null>(null);
  const sessionStartMsRef = useRef<number | null>(null);
  const globalTimeOffsetRef = useRef<number | null>(null);

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

    // load initial sessions
    loadSessions();
    connectionRef.current = connection;

    connection.start().then(() => setIsConnected(true));

    // Ko prejmemo signal, da se je snemanje začelo
    connection.on("RecordingStarted", (data: { id: string, name: string }) => {
      sessionStartRealRef.current = Date.now(); // Trenutni čas v brskalniku (Unix ms)
      sessionStartMsRef.current = null; // Resetiramo, da ujamemo prvi paket
      console.log("Snemanje se je začelo ob:", new Date(sessionStartRealRef.current).toISOString());
    });

    // V connection.on("ReceiveMeasurement", ...) zamenjaj celotno logiko za čas s temle:
    connection.on("ReceiveMeasurement", (msg: any) => {
      setLastValue(msg.value);

      // 1. Izračunaj globalni offset ob prvem paketu (Wall Clock Sync)
      if (globalTimeOffsetRef.current === null) {
          // Trenutni čas v ms MINUS milisekunde iz ESP32
          globalTimeOffsetRef.current = Date.now() - msg.timestampMs;
          console.log("Sinhronizacija časa končana. Offset:", globalTimeOffsetRef.current);
      }

      // 2. Izračunaj realni čas za to točko
      // (ESP32 ms + offset) / 1000 = sekunde od 1970 (Unix Epoch)
      const displayTime = (msg.timestampMs + globalTimeOffsetRef.current) / 1000;

      xDataRef.current.push(displayTime);
      yDataRef.current.push(msg.value);

      // 2. KLJUČNO: Omeji dolžino polja (npr. na zadnjih 200 točk)
      const MAX_POINTS = 200; 
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

      // Wait to load sessions after stop recording and then list them
      setTimeout(() => {
        loadSessions();
      }, 500);
    }
  };

  // 1. Dodaj state za seje
  const [sessions, setSessions] = useState<any[]>([]);

  // 2. Funkcija za osveževanje (pokliči jo v useEffect ali ob StopRecording)
  const loadSessions = async () => {
    try {
      const r = await fetch("http://localhost:5247/api/telemetry/sessions");
      const data = await r.json();
      setSessions(data);
    } catch (e) {
      console.error("Napaka pri branju sej", e);
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

      <div className="mt-10 border-t border-slate-800 pt-6">
        <h2 className="text-xl font-bold mb-4 text-slate-400">Pretekle meritve</h2>
        <div className="flex flex-col gap-2">
          {sessions.map(s => (
            <div key={s.id} className="p-3 bg-slate-900 rounded border border-slate-800 flex justify-between">
              <span>{s.name}</span>
              <span className="text-slate-500 text-sm">{new Date(s.createdAt).toLocaleString()}</span>
            </div>
          ))}
        </div>
      </div>
    </div>
  );
}