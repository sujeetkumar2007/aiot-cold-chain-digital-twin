from fastapi.middleware.cors import CORSMiddleware
from fastapi import FastAPI, Depends, HTTPException
from sqlalchemy.orm import Session

import models
import schemas
import crud

from database import engine, get_db, Base

Base.metadata.create_all(bind=engine)

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
