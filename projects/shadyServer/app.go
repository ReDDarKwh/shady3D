package main

import (
	"context"
	"fmt"
	"math"
	"net"
	"os"
	"path/filepath"
	"sync"
	"time"

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

func exit(format string, a ...interface{}) {
	fmt.Fprintf(os.Stderr, filepath.Base(os.Args[0])+": "+format+"\n", a...)
	os.Exit(1)
}

func printTime(s string, args ...interface{}) {
	fmt.Printf(time.Now().Format("15:04:05.0000")+" "+s+"\n", args...)
}

func dedup(conn *net.Conn, paths ...string) {
	if len(paths) < 1 {
		exit("must specify at least one path to watch")
	}

	// Create a new watcher.
	w, err := fsnotify.NewWatcher()
	if err != nil {
		exit("creating a new watcher: %s", err)
	}
	defer w.Close()

	// Start listening for events.
	go dedupLoop(w, conn)

	// Add all paths from the commandline.
	for _, p := range paths {
		err = w.Add(p)
		if err != nil {
			exit("%q: %s", p, err)
		}
	}

	printTime("ready; press ^C to exit")
	<-make(chan struct{}) // Block forever
}

func dedupLoop(w *fsnotify.Watcher, conn *net.Conn) {
	var (
		waitFor = 8 * time.Millisecond

		// Keep track of the timers, as path → timer.
		mu     sync.Mutex
		timers = make(map[string]*time.Timer)

		// Callback we run.
		printEvent = func(e fsnotify.Event) {
			printTime(e.String())

			if (*conn) != nil {
				(*conn).Write([]byte(e.Name + "\n"))
			}

			// Don't need to remove the timer if you don't have a lot of files.
			mu.Lock()
			delete(timers, e.Name)
			mu.Unlock()
		}
	)

	for {
		select {
		// Read from Errors.
		case err, ok := <-w.Errors:
			if !ok { // Channel was closed (i.e. Watcher.Close() was called).
				return
			}
			printTime("ERROR: %s", err)
		// Read from Events.
		case e, ok := <-w.Events:
			if !ok { // Channel was closed (i.e. Watcher.Close() was called).
				return
			}

			// We just want to watch for file creation, so ignore everything
			// outside of Create and Write.
			if !e.Has(fsnotify.Create) && !e.Has(fsnotify.Write) {
				continue
			}

			// Get timer.
			mu.Lock()
			t, ok := timers[e.Name]
			mu.Unlock()

			// No timer yet, so create one.
			if !ok {
				t = time.AfterFunc(math.MaxInt64, func() { printEvent(e) })
				t.Stop()

				mu.Lock()
				timers[e.Name] = t
				mu.Unlock()
			}

			// Reset the timer for this path, so it will start from 100ms again.
			t.Reset(waitFor)
		}
	}
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

	// watcher, err := fsnotify.NewWatcher()
	// if err != nil {
	// 	log.Fatal(err)
	// }

	// err = watcher.Add("C:/Github/webgpucpp/projects/client/shaders")
	// if err != nil {
	// 	fmt.Println(err)
	// }

	// defer watcher.Close()

	// // Start listening for events.
	// for {
	// 	select {
	// 	case event, ok := <-watcher.Events:
	// 		if !ok {
	// 			return
	// 		}
	// 		fmt.Println("event:", event)
	// 		if event.Has(fsnotify.Write) {

	// 			fmt.Println("modified file:", event.Name)
	// 			if conn != nil {
	// 				conn.Write([]byte(event.Name + "\n"))
	// 			}
	// 		}
	// 	case err, ok := <-watcher.Errors:
	// 		if !ok {
	// 			return
	// 		}
	// 		fmt.Println("error:", err)
	// 	}
	// }

	dedup(&conn, "C:/Github/webgpucpp/projects/client/shaders")
}

func (a *App) shutdown(ctx context.Context) {

}

// Greet returns a greeting for the given name
func (a *App) Greet(name string) string {
	return fmt.Sprintf("Hello %s, It's showww time!", name)
}
