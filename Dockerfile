FROM git.aurora.aur/aurora/cpp

COPY . .

RUN cmake --workflow --preset=docker
