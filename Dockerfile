# Build from a basic Ubuntu image
FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# Install system dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    python3 \
    python3-pip \
    libncurses5-dev \
    libncursesw5-dev \
    openmpi-bin \
    libopenmpi-dev \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy the current directory contents into the container at /app
COPY . .

# Build C binaries
RUN make

# Default entrypoint
ENTRYPOINT [ "python3", "benchmark.py" ]