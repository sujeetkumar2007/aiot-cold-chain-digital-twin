
import joblib
import numpy as np

from functools import lru_cache
from .train import MODEL_PATH, train_model


@lru_cache(maxsize=1)
def get_model():
    if not MODEL_PATH.exists():
        return train_model()

    saved = joblib.load(MODEL_PATH)
    return saved["model"]


def analyze_reading(temperature, humidity, gas_raw=None, door_open=0):
    model = get_model()

    # The ML model analyses temperature and humidity.
    features = np.array([[temperature, humidity]])

    prediction = int(model.predict(features)[0])
    score = float(model.decision_function(features)[0])

    is_anomaly = prediction == -1

    flags = []

    if door_open == 1:
        flags.append("Storage door is currently open")

    return {
        "model": "Isolation Forest",
        "is_anomaly": is_anomaly,
        "status": "ANOMALY" if is_anomaly else "NORMAL",
        "anomaly_score": round(-score, 4),
        "temperature": temperature,
        "humidity": humidity,
        "gas_raw": gas_raw,
        "door_status": "OPEN" if door_open else "CLOSED",
        "additional_flags": flags,
        "gas_sensor_note": (
            "Raw ADC value only; not calibrated to gas concentration."
            if gas_raw is not None else None
        ),
        "training_data": "Synthetic illustrative cold-storage baseline"
    }
