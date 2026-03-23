"use client";

import React, { useEffect, useState } from 'react';
import { useParams, useRouter } from 'next/navigation';
import UplotReact from 'uplot-react';
import 'uplot/dist/uPlot.min.css';

export default function AnalyticsPage() {
  const params = useParams();
  const router = useRouter();
  const [chartData, setChartData] = useState<[number[], number[]] | null>(null);
  const [loading, setLoading] = useState(true);

  const options: any = { // Za začetek uporabi 'any', da uideš strogi validaciji, ali pa spodnji fiks:
    width: 1000,
    height: 500,
    title: `Seja: ${params.sessionId}`,
    series: [
        {},
        {
        label: "CAN Signal",
        stroke: "#3b82f6",
        width: 2,
        },
    ],
    axes: [
        { stroke: "#64748b" },
        { stroke: "#64748b" }
    ],
    cursor: {
        drag: {
        setScale: true,    // To dejansko poveča graf ob spustu miške
        setSelect: true,   // To nariše modri/sivi pravokotnik med vlečenjem
        x: true,           // Zoomiramo po X osi
        y: false,          // Po navadi ne želimo zoomirati Y osi, da ostane skala fiksna
        }
    },

    hooks: {
        ready: [
            (u: any) => {
            u.over.addEventListener("dblclick", () => {
                u.setData(chartData); // Resetira na originalne podatke in skalo
            });
            }
        ]
    }
    };

  useEffect(() => {
    const fetchHistory = async () => {
      try {
        const r = await fetch(`http://localhost:5247/api/telemetry/sessions/${params.sessionId}/measurements`);
        const data = await r.json();
        setChartData(data);
      } catch (e) {
        console.error("Napaka pri nalaganju:", e);
      } finally {
        setLoading(false);
      }
    };

    fetchHistory();
  }, [params.sessionId]);

  return (
    <div className="p-8 bg-slate-950 min-h-screen text-white">
      <button 
        onClick={() => router.push('/')}
        className="mb-6 text-blue-400 hover:text-blue-300 flex items-center gap-2"
      >
        ← Nazaj na Live Dashboard
      </button>

      <div className="bg-slate-900 p-6 rounded-xl border border-slate-800">
        {loading ? (
          <p className="animate-pulse text-slate-500 text-center py-20">Nalagam tisoče točk iz baze...</p>
        ) : chartData ? (
          <UplotReact options={options} data={chartData} />
        ) : (
          <p className="text-red-400 text-center py-20">Podatkov za to sejo ni bilo mogoče najti.</p>
        )}
      </div>
    </div>
  );
}