"use client";

import React, { useEffect, useState, useRef } from 'react';
import * as signalR from "@microsoft/signalr";
import UplotReact from 'uplot-react';
import 'uplot/dist/uPlot.min.css';

export default function LiveDashboard() {
  const [isConnected, setIsConnected] = useState(false);
  const [lastValue, setLastValue] = useState(0);
  
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

  return (
    <div className="p-8 bg-slate-950 min-h-screen text-white">
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