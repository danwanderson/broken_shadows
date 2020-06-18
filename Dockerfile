# Use an official Alpine image as base
FROM alpine:latest

RUN mkdir -p /srv/shadows
COPY shadows /srv/shadows

WORKDIR /srv/shadows

ENTRYPOINT ["/srv/shadows/bin/start_shadows.sh"]
