
from sqlalchemy import Column, Integer, Float, DateTime
from sqlalchemy.sql import func

from database import Base


class SensorReading(Base):
    __tablename__ = "sensor_readings"

    id = Column(Integer, primary_key=True, index=True)

    temperature = Column(Float, nullable=False)
    humidity = Column(Float, nullable=False)
    gas_raw = Column(Integer, nullable=True)

    door_open = Column(Integer, nullable=False, default=0)

    risk_score = Column(Float, nullable=True)

    timestamp = Column(
        DateTime(timezone=True),
        server_default=func.now()
    )
