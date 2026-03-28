"use client";

import React, { useEffect, useState, useMemo } from 'react';
import { useParams, useRouter, useSearchParams } from 'next/navigation';
import UplotReact from 'uplot-react';
import 'uplot/dist/uPlot.min.css';

export default function AnalyticsPage() {
  const params = useParams();
  const router = useRouter();
  const searchParams = useSearchParams();
  
  // Initialize with empty arrays to prevent "length" errors
  const [chartData, setChartData] = useState<[number[], number[]]>([[], []]);
  const [loading, setLoading] = useState(true);

  // Read zoom from URL
  const urlMin = searchParams.get('min');
  const urlMax = searchParams.get('max');

  // Points display
  const [pointCount, setPointCount] = useState(0);

  const options = useMemo(() => ({
    width: 1000,
    height: 500,
    title: `Seja: ${params.sessionId}`,
    series: [
        {},
        { label: "CAN Signal", stroke: "#3b82f6", width: 2 },
    ],
    axes: [{ stroke: "#64748b" }, { stroke: "#64748b" }],
    cursor: {
        drag: { setScale: true, setSelect: true, x: true, y: false }
    },
    scales: {
        x: {
            // If URL has min/max, apply them immediately
            min: urlMin ? parseFloat(urlMin) : undefined,
            max: urlMax ? parseFloat(urlMax) : undefined,
        }
    },
    hooks: {
        // When user zooms, update the URL
        setSelect: [
            (u: any) => {
                const min = u.posToVal(u.select.left, 'x').toFixed(4);
                const max = u.posToVal(u.select.left + u.select.width, 'x').toFixed(4);
                router.replace(`?min=${min}&max=${max}`, { scroll: false });
            }
        ],
        ready: [
            (u: any) => {
                u.over.addEventListener("dblclick", () => {
                    router.replace(`${window.location.pathname}`, { scroll: false });
                    u.setData(chartData); 
                });
            }
        ]
    }
  }), [params.sessionId, urlMin, urlMax, chartData]);

  useEffect(() => {
    const fetchHistory = async () => {
      setLoading(true);
      try {
        // Build query string based on URL params
        const query = (urlMin && urlMax) 
          ? `?min=${urlMin}&max=${urlMax}` 
          : "";
          
        const r = await fetch(`http://localhost:5247/api/telemetry/sessions/${params.sessionId}/measurements${query}`);
        const data = await r.json();
        
        if (Array.isArray(data) && data.length === 2) {
          setChartData(data as [number[], number[]]);
          // Track how many points the server sent
          setPointCount(data[0].length);
        }
      } catch (e) {
        console.error("Napaka:", e);
      } finally {
        setLoading(false);
      }
    };

    if (params.sessionId) fetchHistory();
    
    // Depend on urlMin/urlMax so it refetches on zoom!
  }, [params.sessionId, urlMin, urlMax]);

  return (
    <div className="p-8 bg-slate-950 min-h-screen text-white">
      <div className="flex justify-between items-center mb-6">
        <button 
          onClick={() => router.push('/')}
          className="text-blue-400 hover:text-blue-300 flex items-center gap-2"
        >
          ← Back to Live Dashboard
        </button>
        {urlMin && (
          <span className="text-xs bg-blue-500/20 text-blue-400 px-3 py-1 rounded-full border border-blue-500/30">
            Linkable View Active
          </span>
        )}
      </div>

      <div className="flex justify-between mb-2">
        <p className="text-slate-400 text-sm">Showing {pointCount} points</p>
        {urlMin && <p className="text-blue-400 text-sm font-mono">Zoomed Resolution Active</p>}
    </div>

      <div className="bg-slate-900 p-6 rounded-xl border border-slate-800 shadow-2xl">
        {loading ? (
          <p className="animate-pulse text-slate-500 text-center py-20">Loading data...</p>
        ) : chartData[0].length > 0 ? (
          <UplotReact options={options} data={chartData} />
        ) : (
          <p className="text-red-400 text-center py-20">No data for this session</p>
        )}
      </div>
    </div>
  );
}