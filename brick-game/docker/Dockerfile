# PlatformIO in a container, so no host install (or sudo) is needed.
# Built and used by ../pio.sh
FROM python:3.12-slim
RUN apt-get update \
 && apt-get install -y --no-install-recommends git \
 && rm -rf /var/lib/apt/lists/* \
 && pip install --no-cache-dir platformio
ENV PLATFORMIO_CORE_DIR=/cache/platformio
WORKDIR /project
ENTRYPOINT ["pio"]
