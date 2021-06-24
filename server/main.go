package main

import (
	"bytes"
	"container/list"
	"encoding/binary"
	"flag"
	"fmt"
	log "github.com/sirupsen/logrus"
	"net"
	"strconv"
	"time"
)

type NodeAddress [6]byte

type PacketHeader struct {
	DstNode   NodeAddress
	DstSocket uint16
	SrcNode   NodeAddress
	SrcSocket uint16
}

func extractHeader(d []byte) (ret PacketHeader) {
	copy(ret.DstNode[:], d[0:6])
	binary.Read(bytes.NewBuffer(d[6:8]), binary.LittleEndian, &ret.DstSocket)
	copy(ret.SrcNode[:], d[8:14])
	binary.Read(bytes.NewBuffer(d[14:16]), binary.LittleEndian, &ret.SrcSocket)
	return
}

const headerSize = 16

type Client struct {
	Endpoint *net.UDPAddr
	LastSeen time.Time
}

var socketMap map[uint16]map[NodeAddress]*Client
var broadcastAddress NodeAddress

func expired(t *Client) bool {
	return time.Now().Sub(t.LastSeen).Seconds() > 120
}

func displayAddress(a NodeAddress) string {
	return fmt.Sprintf("%02X.%02X.%02X.%02X.%02X.%02X", a[0], a[1], a[2], a[3], a[4], a[5])
}

func handlePacket(conn *net.UDPConn, p []byte, a *net.UDPAddr) {
	if len(p) < headerSize {
		log.Warningf("Packet from %+v is too small(%d)\n", a, len(p))
		return
	}

	header := extractHeader(p)

	log.Debugf("Received packet from UDP %+v, dst %s:%d, src %s:%d\n", a,
		displayAddress(header.DstNode), header.DstSocket,
		displayAddress(header.SrcNode), header.SrcSocket)

	if _, ok := socketMap[header.SrcSocket]; !ok {
		socketMap[header.SrcSocket] = make(map[NodeAddress]*Client)
	}

	if _, ok := socketMap[header.SrcSocket][header.SrcNode]; !ok {
		log.Infof("New client %s:%d from %+v", displayAddress(header.SrcNode), header.SrcSocket, a)
	}
	socketMap[header.SrcSocket][header.SrcNode] = &Client{a, time.Now()}

	if v, ok := socketMap[header.DstSocket]; ok {
		if header.DstNode == broadcastAddress {
			expireList := list.New()
			for k, vv := range v {
				if expired(vv) {
					expireList.PushBack(k)
				}
			}
			for e := expireList.Front(); e != nil; e = e.Next() {
				delete(v, e.Value.(NodeAddress))
			}

			for node, vv := range v {
				log.Debugf("Sending broadcast to %+v (last seen %ds ago)", vv.Endpoint, int(time.Now().Sub(vv.LastSeen).Seconds()))
				if node != header.SrcNode {
					_, err := conn.WriteTo(p, vv.Endpoint)
					if err != nil {
						log.Warningf("Failed to send broadcast packet to %+v: %s", vv.Endpoint, err.Error())
					}
				}
			}
		} else {
			if vv, ok := v[header.DstNode]; ok {
				if expired(vv) {
					delete(v, header.DstNode)
				} else {
					vv.LastSeen = time.Now()
					log.Debugf("Sending unicast to %+v (last seen %ds ago)", vv.Endpoint, int(time.Now().Sub(vv.LastSeen).Seconds()))
					_, err := conn.WriteTo(p, vv.Endpoint)
					if err != nil {
						log.Warningf("Failed to send unicast packet to %+v: %s", vv.Endpoint, err.Error())
					}
				}
			}
		}
	}
}

func main() {
	verbose := flag.Bool("v", false, "Verbose logging")
	port := flag.Int("p", 8899, "Port to listen")
	flag.Parse()

	if *verbose {
		log.SetLevel(log.DebugLevel)
	} else {
		log.SetLevel(log.InfoLevel)
	}

	broadcastAddress = [6]byte{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}
	socketMap = make(map[uint16]map[NodeAddress]*Client)

	udpStr := ":" + strconv.Itoa(*port)
	udpAddr, _ := net.ResolveUDPAddr("udp", udpStr)
	conn, err := net.ListenUDP("udp", udpAddr)
	if err != nil {
		log.Errorf("Unable to listen on udp %s", udpStr)
		return
	}
	log.Infof("Listening on %s", udpAddr)

	for {
		data := make([]byte, 1500)
		n, a, err := conn.ReadFromUDP(data)
		if err != nil {
			log.Warningf("failed read udp msg, error: %s", err.Error())
			continue
		}

		handlePacket(conn, data[:n], a)
	}
}
