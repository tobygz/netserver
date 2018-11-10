package main

import (
	"fmt"
	//"os"
	"bytes"
	"encoding/binary"
	"net"
	"time"
)

func checkError(err error, info string) (res bool) {
	if err != nil {
		fmt.Println("[info] " + info + "  [err] " + err.Error())
		return false
	}
	return true
}

func IntToByte(num int32) []byte {
	var buffer bytes.Buffer
	err := binary.Write(&buffer, binary.LittleEndian, num)
	checkError(err, "IntToByte")
	return buffer.Bytes()
}

func ReadLen(con net.Conn, slc []byte, lenv int) bool {
	cacLen := 0
	for {
		rlen, err := con.Read(slc)
		if checkError(err, "Readlen") == false {
			con.Close()
			return false
			//os.Exit(0)
		}
		cacLen += rlen
		if cacLen >= lenv {
			return true
		}
	}
	return true
}

func chatSend(conn net.Conn) {
	//buf := make([]byte,1024)
	/*
	   for i:=0;i<1024;i++{
	       buf[i] = byte(i);
	   }*/
	buf := []byte("hello,world")
	lenv := len(buf)
	msgid := uint32(1008)
	len_bytes := IntToByte(int32(lenv))
	msg_bytes := IntToByte(int32(msgid))

	tick := time.NewTicker(80 * time.Millisecond)
	for {
		select {
		case <-tick.C:
			for i := 0; i < 128; i++ {
				_, err0 := conn.Write(len_bytes)
				if checkError(err0, "Connection0") == false {
					conn.Close()
					return
					//os.Exit(0)
				}

				_, err1 := conn.Write(msg_bytes)
				if checkError(err1, "Connection1") == false {
					conn.Close()
					return
					//os.Exit(0)
				}

				_, err2 := conn.Write(buf)
				if checkError(err2, "Connection2") == false {
					conn.Close()
					return
					//os.Exit(0)
				}
			}
			//fmt.Println("send len0:", len0," len1:", len1, " len2:",len2 )
		}
	}
}

func chatRecv(con net.Conn) {
	buf := make([]byte, 20480)
	var bodylen int32 = int32(0)
	msgid := uint32(0)
	for {
		ret := ReadLen(con, buf[:4], 4)
		if !ret {
			con.Close()
			return
		}
		bin_buf := bytes.NewBuffer(buf[:4])
		binary.Read(bin_buf, binary.LittleEndian, &bodylen)

		ret = ReadLen(con, buf[:4], 4)
		if !ret {
			con.Close()
			return
		}

		bin_buf = bytes.NewBuffer(buf[:4])
		binary.Read(bin_buf, binary.LittleEndian, &msgid)

		if bodylen > 20 {
			con.Close()
			return
		}

		ReadLen(con, buf[:bodylen], int(bodylen))
		//fmt.Println("recv ", string(buf[0:bodylen]), " len: ", bodylen, ", msgid:", msgid)
		//time.Sleep(time.Second)
	}
}

func startCli(tcpaddr string) {
	tcpAddr, err := net.ResolveTCPAddr("tcp4", tcpaddr)
	checkError(err, "ResolveTCPAddr")
	conn, err := net.DialTCP("tcp", nil, tcpAddr)
	checkError(err, "DialTCP")

	go chatSend(conn)
	go chatRecv(conn)
	//time.Sleep(time.Hour)
}

func main() {
	for i := 0; i < 500; i++ {
		go startCli("127.0.0.1:6010")
		//time.Sleep(time.Second)
	}
	time.Sleep(time.Hour)
	/*
	   for i,v := range IntToByte(int32(1)) {
	       fmt.Println(i," ",v)
	   }
	*/
}
