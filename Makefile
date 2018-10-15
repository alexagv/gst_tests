CFLAGS ?= -O2 -g -Wall
LDFLAGS ?=

GST_CFLAGS = $(shell pkg-config --cflags gstreamer-1.0)
GST_LDFLAGS = $(shell pkg-config --libs gstreamer-1.0)

all: fec-client fec-server test-udp-sender test-udp-reciever

clean: fec-client fec-server test-udp-sender test-udp-reciever

fec-client: fec-client.c
	$(CC) -o $@ $^ $(CFLAGS) $(GST_CFLAGS) $(LDFLAGS) $(GST_LDFLAGS)

fec-server: fec-server.c
	$(CC) -o $@ $^ $(CFLAGS) $(GST_CFLAGS) $(LDFLAGS) $(GST_LDFLAGS)

test-udp-sender: test-udp-sender.c
	$(CC) -o $@ $^ $(CFLAGS) $(GST_CFLAGS) $(LDFLAGS) $(GST_LDFLAGS)

test-udp-reciever: test-udp-reciever.c
	$(CC) -o $@ $^ $(CFLAGS) $(GST_CFLAGS) $(LDFLAGS) $(GST_LDFLAGS)
