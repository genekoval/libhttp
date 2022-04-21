package main

import (
	"encoding/json"
	"fmt"
	"io/ioutil"
	"log"
	"net/http"
)

type Book struct {
	Title  string `json:"title"`
	Author string `json:"author"`
	Year   int    `json:"year"`
}

func main() {
	http.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
		fmt.Fprintf(w, "A Test Server!")
	})

	http.HandleFunc("/echo", func(w http.ResponseWriter, r *http.Request) {
		defer r.Body.Close()

		body, _ := ioutil.ReadAll(r.Body)
		text := string(body)

		fmt.Fprintf(w, "Your request (%s): %s", r.Method, text)
	})

	http.HandleFunc("/json", func(w http.ResponseWriter, r *http.Request) {
		book := Book{"Hello Golang", "John Mike", 2021}
		bytes, _ := json.Marshal(book)
		w.Write(bytes)
	})

	log.Fatal(http.ListenAndServe("127.0.0.1:8080", nil))
}
