
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
  const [error, setError] = useState(false)
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
      } catch (err) {
        setError(true)
      }
    }

    fetchData()
    const interval = setInterval(fetchData, 5000)

    return () => clearInterval(interval)
  }, [])

  return (
    <div className="dashboard">
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
        <section className="welcome">
          <h1>Storage Dashboard</h1>
          <p>Real-time cold storage health overview</p>
          <small>Last updated: {lastUpdated}</small>
        </section>

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

        <section className="health-card">
          <h3>Storage Risk Score</h3>
          <h1>{data ? `${data.risk_score} / 100` : '-- / 100'}</h1>
          <p>Preliminary demo score, not a validated food-safety metric.</p>
        </section>

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
