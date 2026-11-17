
from pydantic import BaseModel, Field
from datetime import datetime
from typing import Optional


class SensorReadingCreate(BaseModel):
    temperature: float
    humidity: float
    gas_raw: Optional[int] = None
    door_open: int = Field(default=0, ge=0, le=1)
    risk_score: Optional[float] = None


class SensorReadingResponse(BaseModel):
    id: int
    temperature: float
    humidity: float
    gas_raw: Optional[int]
    door_open: int
    risk_score: Optional[float]
    timestamp: datetime

    class Config:
        from_attributes = True
