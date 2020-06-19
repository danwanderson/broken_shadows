FROM debian:latest

RUN apt-get update && apt-get install -y telnet gdb make clang

RUN mkdir -p /srv/shadows

RUN useradd --no-create-home --home-dir /srv/shadows shadows

COPY area /srv/shadows/area
COPY bin /srv/shadows/bin
COPY gods /srv/shadows/gods
COPY log /srv/shadows/log
COPY notes /srv/shadows/notes
COPY player /srv/shadows/player
COPY src /srv/shadows/src

RUN chown -R shadows /srv/shadows

USER shadows
WORKDIR /srv/shadows

ENTRYPOINT ["/srv/shadows/bin/start_shadows.sh"]
#ENTRYPOINT ["/bin/bash"]
