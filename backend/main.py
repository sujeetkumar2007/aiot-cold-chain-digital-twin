
import asyncio

from fastapi.middleware.cors import CORSMiddleware
from fastapi import FastAPI, Depends, HTTPException
from sqlalchemy.orm import Session

import models
import schemas
import crud

from database import engine, get_db, Base


Base.metadata.create_all(bind=engine)

DISCOVERY_PORT = 4210
DISCOVERY_REQUEST = b"COLDCHAIN_DISCOVER"
DISCOVERY_RESPONSE = b"COLDCHAIN_SERVER"


class DiscoveryProtocol(asyncio.DatagramProtocol):

    def connection_made(self, transport):
        self.transport = transport
        print("UDP discovery service started on port 4210")

    def datagram_received(self, data, addr):
        if data.strip() == DISCOVERY_REQUEST:
            print(f"ESP32 discovery request received from {addr[0]}")
            self.transport.sendto(DISCOVERY_RESPONSE, addr)
            print(f"Discovery response sent to {addr[0]}")


app = FastAPI(
    title="AIoT Cold-Chain Backend",
    description="Cold storage monitoring and predictive analytics API",
    version="1.0.0"
)


app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=False,
    allow_methods=["*"],
    allow_headers=["*"],
)


@app.on_event("startup")
async def start_discovery():
    loop = asyncio.get_running_loop()

    transport, protocol = await loop.create_datagram_endpoint(
        lambda: DiscoveryProtocol(),
        local_addr=("0.0.0.0", DISCOVERY_PORT)
    )

    app.state.discovery_transport = transport


@app.on_event("shutdown")
async def stop_discovery():
    transport = getattr(app.state, "discovery_transport", None)

    if transport:
        transport.close()


@app.get("/")
def home():
    return {
        "message": "AIoT Cold-Chain Backend is running",
        "status": "online"
    }


@app.post("/api/sensor", response_model=schemas.SensorReadingResponse)
def receive_sensor_data(
    reading: schemas.SensorReadingCreate,
    db: Session = Depends(get_db)
):
    return crud.create_sensor_reading(db, reading)


@app.get("/api/latest", response_model=schemas.SensorReadingResponse)
def latest_reading(db: Session = Depends(get_db)):
    reading = crud.get_latest_sensor_reading(db)

    if reading is None:
        raise HTTPException(
            status_code=404,
            detail="No sensor readings available"
        )

    return reading


@app.get("/api/history")
def sensor_history(
    limit: int = 100,
    db: Session = Depends(get_db)
):
    if limit < 1 or limit > 1000:
        raise HTTPException(
            status_code=400,
            detail="Limit must be between 1 and 1000"
        )

    return crud.get_sensor_readings(db, limit)
