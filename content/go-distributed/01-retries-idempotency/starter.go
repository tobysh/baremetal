package main

import "fmt"

// Request asks the server to add Amount to the shared balance.
// (Client, Seq) is the identity of the request: a retry repeats both.
type Request struct {
	Client string
	Seq    int
	Amount int
}

// Response answers one request. Balance is the balance right after the
// request was applied.
type Response struct {
	Client  string
	Seq     int
	Balance int
}

type Server struct {
	// add your state here
}

func NewServer() *Server {
	return &Server{}
}

func (s *Server) Handle(r Request) Response {
	return Response{}
}

func (s *Server) Balance() int {
	return 0
}

type Client struct {
	// add your state here
}

func NewClient(id string, amounts []int) *Client {
	return &Client{}
}

// Pending returns the request to send now, or false when every request has
// been acknowledged.
func (c *Client) Pending() (Request, bool) {
	return Request{}, false
}

func (c *Client) OnResponse(r Response) {}

func (c *Client) Done() bool {
	return false
}

// Try it: a perfect network, then a duplicated request.
func main() {
	srv := NewServer()
	cl := NewClient("alice", []int{10, -3, 5})
	for !cl.Done() {
		req, _ := cl.Pending()
		resp := srv.Handle(req)
		if req.Seq == 2 {
			srv.Handle(req) // the network delivers request 2 twice
		}
		cl.OnResponse(resp)
	}
	fmt.Println("balance:", srv.Balance()) // 12 when duplicates are ignored
}
