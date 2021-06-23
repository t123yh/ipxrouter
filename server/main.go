package main

import (
	"bytes"
	"container/list"
	"encoding/binary"
	"fmt"
	log "github.com/sirupsen/logrus"
	"net"
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
	return fmt.Sprintf("%X.%X.%X.%X.%X.%X", a[0], a[1], a[2], a[3], a[4], a[5])
}

func handlePacket(conn *net.UDPConn, p []byte, a *net.UDPAddr) {
	if len(p) < headerSize {
		log.Printf("Packet from %+v is too small(%d)\n", a, len(p))
		return
	}

	header := extractHeader(p)

	log.Printf("Received packet from UDP %+v, dst %s:%d, src %s:%d\n", a,
		displayAddress(header.DstNode), header.DstSocket,
		displayAddress(header.SrcNode), header.SrcSocket)

	if _, ok := socketMap[header.SrcSocket]; !ok {
		socketMap[header.SrcSocket] = make(map[NodeAddress]*Client)
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

			for _, vv := range v {
							fmt.Printf("Sending to %+v (last seen %ds ago) \n", vv.Endpoint, int(time.Now().Sub(vv.LastSeen).Seconds()))
				conn.WriteTo(p, vv.Endpoint)
			}
		} else {
			if vv, ok := v[header.DstNode]; ok {
				vv.LastSeen = time.Now()
				fmt.Printf("Sending to %+v\n", vv.Endpoint)
				conn.WriteTo(p, vv.Endpoint)
			}
		}
	}
}

func main() {
	broadcastAddress = [6]byte{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}
	socketMap = make(map[uint16]map[NodeAddress]*Client)

	udpAddr, err := net.ResolveUDPAddr("udp", "0.0.0.0:8899")
	if err != nil {
		panic(err)
	}
	conn, err := net.ListenUDP("udp", udpAddr)

	for {
		data := make([]byte, 1500)
		n, a, err := conn.ReadFromUDP(data)
		if err != nil {
			fmt.Println("failed read udp msg, error: " + err.Error())
			continue
		}

		handlePacket(conn, data[:n], a)
	}
}

