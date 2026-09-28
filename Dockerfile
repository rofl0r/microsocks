FROM alpine AS builder
RUN apk add --no-cache make gcc libc-dev
WORKDIR /app
ENV LDFLAGS=-static
COPY . .
RUN make

FROM scratch
LABEL org.opencontainers.image.licenses=MIT
COPY --from=builder /app/microsocks /
ENTRYPOINT ["/microsocks"]