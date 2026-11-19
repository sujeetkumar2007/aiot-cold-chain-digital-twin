
import { useEffect, useState } from 'react'
import {
  ResponsiveContainer,
  LineChart,
  Line,
  XAxis,
  YAxis,
  CartesianGrid,
  Tooltip,
  Legend
} from 'recharts'
import './App.css'

const API = `http://${window.location.hostname}:8000`

const formatIST = (date) =>
  new Intl.DateTimeFormat('en-IN', {
    timeZone: 'Asia/Kolkata',
    hour: '2-digit',
    minute: '2-digit',
    second: '2-digit',
    hour12: true
  }).format(date)

const parseBackendTime = (timestamp) => {
  const hasTimezone = /Z$|[+-]\d{2}:\d{2}$/.test(timestamp)
  return new Date(hasTimezone ? timestamp : `${timestamp}Z`)
}

function App() {
  const [data, setData] = useState(null)
  const [history, setHistory] = useState([])
  const [anomaly, setAnomaly] = useState(null)

  const [error, setError] = useState(false)
  const [mlError, setMlError] = useState(false)

  const [lastUpdated, setLastUpdated] = useState('--')

  useEffect(() => {
    const fetchData = async () => {
      try {
        const [latestResponse, historyResponse] = await Promise.all([
          fetch(`${API}/api/latest`),
          fetch(`${API}/api/history`)
        ])

        if (!latestResponse.ok || !historyResponse.ok) {
          throw new Error('Backend unavailable')
        }

        const latest = await latestResponse.json()
        const records = await historyResponse.json()

        setData(latest)

        const sortedRecords = records
          .slice()
          .sort((a, b) =>
            new Date(a.timestamp) - new Date(b.timestamp)
          )
          .slice(-60)
          .map(record => ({
            ...record,
            time: formatIST(parseBackendTime(record.timestamp))
          }))

        setHistory(sortedRecords)
        setError(false)
        setLastUpdated(formatIST(new Date()))

        // Fetch live AI anomaly analysis separately
        try {
          const anomalyResponse = await fetch(
            `${API}/api/ml/anomaly/latest`
          )

          if (!anomalyResponse.ok) {
            throw new Error('ML endpoint unavailable')
          }

          const anomalyResult = await anomalyResponse.json()
          setAnomaly(anomalyResult)
          setMlError(false)

        } catch (mlErr) {
          console.error('ML Error:', mlErr)
          setMlError(true)
        }

      } catch (err) {
        console.error('Backend Error:', err)
        setError(true)
      }
    }

    fetchData()

    const interval = setInterval(fetchData, 5000)

    return () => clearInterval(interval)
  }, [])

  const isAnomaly = anomaly?.is_anomaly === true

  return (
    <div className="dashboard">

      {/* HEADER */}
      <header className="topbar">
        <div>
          <h2>ColdChain AI</h2>
          <p>Intelligent Storage Monitoring</p>
        </div>

        <span className="status">
          {error ? '● Backend Offline' : '● System Online'}
        </span>
      </header>

      <main>

        {/* WELCOME */}
        <section className="welcome">
          <h1>Storage Dashboard</h1>
          <p>Real-time cold storage health overview</p>
          <small>Last updated: {lastUpdated}</small>
        </section>

        {/* LIVE SENSOR METRICS */}
        <section className="metrics">

          <div className="metric-card">
            <span>Temperature</span>
            <h2>{data ? `${data.temperature} °C` : '-- °C'}</h2>
            <p>DS18B20 Sensor</p>
          </div>

          <div className="metric-card">
            <span>Humidity</span>
            <h2>{data ? `${data.humidity} %` : '-- %'}</h2>
            <p>DHT11 Sensor</p>
          </div>

          <div className="metric-card">
            <span>Air Quality</span>
            <h2>{data ? data.gas_raw : '--'}</h2>
            <p>MQ-135 Raw Reading</p>
          </div>

          <div className="metric-card">
            <span>Door Status</span>
            <h2>
              {data
                ? data.door_open === 1 ? 'OPEN' : 'CLOSED'
                : '--'}
            </h2>
            <p>Reed Switch</p>
          </div>

        </section>

        {/* STORAGE RISK SCORE */}
        <section className="health-card">
          <h3>Storage Risk Score</h3>
          <h1>{data ? `${data.risk_score ?? '--'} / 100` : '-- / 100'}</h1>
          <p>
            Preliminary demo score, not a validated food-safety metric.
          </p>
        </section>

        {/* AI ANOMALY DETECTION */}
        <section
          className="health-card"
          style={{
            border: `1px solid ${
              mlError
                ? '#64748b'
                : isAnomaly
                  ? '#ef4444'
                  : '#22c55e'
            }`,
            marginTop: '20px'
          }}
        >
          <h3>AI Storage Health Analysis</h3>

          {mlError ? (
            <>
              <h2 style={{ color: '#f59e0b' }}>
                AI Analysis Unavailable
              </h2>
              <p>
                Could not retrieve the latest anomaly analysis.
                Check the ML endpoint.
              </p>
            </>
          ) : anomaly ? (
            <>
              <h1
                style={{
                  color: isAnomaly ? '#ef4444' : '#22c55e'
                }}
              >
                {anomaly.status}
              </h1>

              <p>
                Model: {anomaly.model}
              </p>

              <p>
                Anomaly Score: {anomaly.anomaly_score}
              </p>

              <p>
                Temperature: {anomaly.temperature} °C
              </p>

              <p>
                Humidity: {anomaly.humidity} %
              </p>

              <p>
                Door: {anomaly.door_status}
              </p>

              {anomaly.additional_flags?.length > 0 && (
                <div style={{ marginTop: '12px' }}>
                  <h4>AI Alerts</h4>

                  {anomaly.additional_flags.map((flag, index) => (
                    <p key={index} style={{ color: '#f87171' }}>
                      ⚠ {flag}
                    </p>
                  ))}
                </div>
              )}

              {isAnomaly && (
                <p style={{ color: '#f87171', fontWeight: 'bold' }}>
                  ⚠ Unusual storage conditions detected.
                  Please inspect the storage environment.
                </p>
              )}

              {!isAnomaly && (
                <p style={{ color: '#4ade80' }}>
                  Storage conditions are within the model's learned
                  baseline.
                </p>
              )}

              <small>
                Model uses a synthetic illustrative training baseline.
                Anomaly score is not a probability.
              </small>
            </>
          ) : (
            <p>Waiting for AI analysis...</p>
          )}
        </section>

        {/* TEMPERATURE HISTORY */}
        <section className="history-card">
          <h3>Temperature History</h3>
          <p>Last 60 recorded readings</p>

          <div className="chart">
            <ResponsiveContainer width="100%" height={280}>
              <LineChart data={history}>
                <CartesianGrid strokeDasharray="3 3" stroke="#334155" />
                <XAxis
                  dataKey="time"
                  tick={{ fill: '#94a3b8', fontSize: 11 }}
                  minTickGap={35}
                />
                <YAxis
                  tick={{ fill: '#94a3b8', fontSize: 11 }}
                  domain={['auto', 'auto']}
                />
                <Tooltip />
                <Legend />
                <Line
                  type="monotone"
                  dataKey="temperature"
                  name="Temperature °C"
                  stroke="#38bdf8"
                  dot={false}
                  strokeWidth={2}
                />
              </LineChart>
            </ResponsiveContainer>
          </div>
        </section>

        {/* HUMIDITY HISTORY */}
        <section className="history-card">
          <h3>Humidity History</h3>
          <p>Last 60 recorded readings</p>

          <div className="chart">
            <ResponsiveContainer width="100%" height={280}>
              <LineChart data={history}>
                <CartesianGrid strokeDasharray="3 3" stroke="#334155" />
                <XAxis
                  dataKey="time"
                  tick={{ fill: '#94a3b8', fontSize: 11 }}
                  minTickGap={35}
                />
                <YAxis
                  tick={{ fill: '#94a3b8', fontSize: 11 }}
                  domain={['auto', 'auto']}
                />
                <Tooltip />
                <Legend />
                <Line
                  type="monotone"
                  dataKey="humidity"
                  name="Humidity %"
                  stroke="#a78bfa"
                  dot={false}
                  strokeWidth={2}
                />
              </LineChart>
            </ResponsiveContainer>
          </div>
        </section>

        {/* AIR QUALITY HISTORY */}
        <section className="history-card">
          <h3>Air Quality History</h3>
          <p>MQ-135 raw sensor readings</p>

          <div className="chart">
            <ResponsiveContainer width="100%" height={280}>
              <LineChart data={history}>
                <CartesianGrid strokeDasharray="3 3" stroke="#334155" />
                <XAxis
                  dataKey="time"
                  tick={{ fill: '#94a3b8', fontSize: 11 }}
                  minTickGap={35}
                />
                <YAxis
                  tick={{ fill: '#94a3b8', fontSize: 11 }}
                  domain={['auto', 'auto']}
                />
                <Tooltip />
                <Legend />
                <Line
                  type="monotone"
                  dataKey="gas_raw"
                  name="MQ-135 Raw"
                  stroke="#4ade80"
                  dot={false}
                  strokeWidth={2}
                />
              </LineChart>
            </ResponsiveContainer>
          </div>
        </section>

        {/* BACKEND ERROR */}
        {error && (
          <p style={{ color: '#f87171' }}>
            Cannot connect to FastAPI. Check that your backend is running.
          </p>
        )}

      </main>

      <footer>
        AIoT Predictive Cold-Chain Digital Twin
      </footer>

    </div>
  )
}

export default App
