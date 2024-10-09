package main

import (
	"embed"
	"fmt"
	"net"

	"github.com/wailsapp/wails/v2"
	"github.com/wailsapp/wails/v2/pkg/options"
	"github.com/wailsapp/wails/v2/pkg/options/assetserver"
)

//go:embed all:frontend/dist
var assets embed.FS

func main() {
	// Create an instance of the app structure
	app := NewApp()

	// Create application with options
	err := wails.Run(&options.App{
		Title:  "shadyServer",
		Width:  1024,
		Height: 768,
		AssetServer: &assetserver.Options{
			Assets: assets,
		},
		BackgroundColour: &options.RGBA{R: 27, G: 38, B: 54, A: 1},
		OnStartup:        app.startup,
		Bind: []interface{}{
			app,
		},
	})

	if err != nil {
		println("Error:", err.Error())
	}

	// ln, err := net.Listen("tcp", ":8080")
	// if err != nil {
	// 	fmt.Println(err)
	// 	return
	// }

	// // Accept incoming connections and handle them
	// for {
	// 	conn, err := ln.Accept()
	// 	if err != nil {
	// 		fmt.Println(err)
	// 		continue
	// 	}

	// 	// Handle the connection in a new goroutine
	// 	go handleConnection(conn)
	// }
}

func handleConnection(conn net.Conn) {
	// Close the connection when we're done
	defer conn.Close()

	// Read incoming data
	buf := make([]byte, 1024)
	_, err := conn.Read(buf)
	if err != nil {
		fmt.Println(err)
		return
	}

	// Print the incoming data
	fmt.Printf("Received: %s", buf)
}

// package main

// import (
// 	"fmt"
// 	"net"
// )

// func main() {
// 	// Listen for incoming connections on port 8080
// 	ln, err := net.Listen("tcp", ":8080")
// 	if err != nil {
// 		fmt.Println(err)
// 		return
// 	}

// 	// Accept incoming connections and handle them
// 	for {
// 		conn, err := ln.Accept()
// 		if err != nil {
// 			fmt.Println(err)
// 			continue
// 		}

// 		// Handle the connection in a new goroutine
// 		go handleConnection(conn)
// 	}
// }

// func handleConnection(conn net.Conn) {
// 	// Close the connection when we're done
// 	defer conn.Close()

// 	// Read incoming data
// 	buf := make([]byte, 1024)
// 	_, err := conn.Read(buf)
// 	if err != nil {
// 		fmt.Println(err)
// 		return
// 	}

// 	// Print the incoming data
// 	fmt.Printf("Received: %s", buf)
// }
