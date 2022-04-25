package main

import (
	"fmt"
	"io/ioutil"
	"log"
	"net/http"
)

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

	log.Fatal(http.ListenAndServe("127.0.0.1:8080", nil))
}
