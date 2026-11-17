
from sqlalchemy.orm import Session
import models
import schemas


def create_sensor_reading(db: Session, reading: schemas.SensorReadingCreate):
    db_reading = models.SensorReading(
        temperature=reading.temperature,
        humidity=reading.humidity,
        gas_raw=reading.gas_raw,
        door_open=reading.door_open,
        risk_score=reading.risk_score
    )

    db.add(db_reading)
    db.commit()
    db.refresh(db_reading)

    return db_reading


def get_sensor_readings(db: Session, limit: int = 100):
    return (
        db.query(models.SensorReading)
        .order_by(models.SensorReading.id.desc())
        .limit(limit)
        .all()
    )


def get_latest_sensor_reading(db: Session):
    return (
        db.query(models.SensorReading)
        .order_by(models.SensorReading.id.desc())
        .first()
    )

