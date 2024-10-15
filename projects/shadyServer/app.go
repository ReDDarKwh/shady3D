package main

import (
	"context"
	"fmt"
	"log"
	"net"

	"github.com/fsnotify/fsnotify"
)

// App struct
type App struct {
	ctx context.Context
}

// NewApp creates a new App application struct
func NewApp() *App {
	return &App{}
}

// startup is called when the app starts. The context is saved
// so we can call the runtime methods
func (a *App) startup(ctx context.Context) {
	a.ctx = ctx

	ln, err := net.Listen("tcp", ":43957")
	if err != nil {
		fmt.Println(err)
		return
	}

	var conn net.Conn
	// Accept incoming connections and handle them
	go func() {
		for {

			newConn, err := ln.Accept()

			if conn != nil {
				conn.Close()
			}

			conn = newConn

			if err != nil {
				fmt.Println(err)
			}
		}
	}()

	watcher, err := fsnotify.NewWatcher()
	if err != nil {
		log.Fatal(err)
	}

	err = watcher.Add("C:/Github/webgpucpp/projects/client/shaders")
	if err != nil {
		fmt.Println(err)
	}

	defer watcher.Close()

	// Start listening for events.
	for {
		select {
		case event, ok := <-watcher.Events:
			if !ok {
				return
			}
			fmt.Println("event:", event)
			if event.Has(fsnotify.Write) {

				fmt.Println("modified file:", event.Name)
				if conn != nil {
					conn.Write([]byte(event.Name + "\n"))
				}
			}
		case err, ok := <-watcher.Errors:
			if !ok {
				return
			}
			fmt.Println("error:", err)
		}
	}
}

func (a *App) shutdown(ctx context.Context) {

}

// Greet returns a greeting for the given name
func (a *App) Greet(name string) string {
	return fmt.Sprintf("Hello %s, It's showww time!", name)
}
