package main

import "fmt"

type Request struct {
	Client string
	Seq    int
	Amount int
}

type Response struct {
	Client  string
	Seq     int
	Balance int
}

type applied struct {
	seq  int
	resp Response
}

type Server struct {
	balance int
	last    map[string]applied
}

func NewServer() *Server {
	return &Server{last: map[string]applied{}}
}

func (s *Server) Handle(r Request) Response {
	a := s.last[r.Client]
	switch {
	case r.Seq == a.seq+1:
		s.balance += r.Amount
		resp := Response{r.Client, r.Seq, s.balance}
		s.last[r.Client] = applied{r.Seq, resp}
		return resp
	case r.Seq == a.seq && a.seq > 0:
		return a.resp
	default:
		return Response{r.Client, r.Seq, s.balance}
	}
}

func (s *Server) Balance() int {
	return s.balance
}

type Client struct {
	id      string
	amounts []int
	acked   int
}

func NewClient(id string, amounts []int) *Client {
	return &Client{id: id, amounts: amounts}
}

func (c *Client) Pending() (Request, bool) {
	if c.acked >= len(c.amounts) {
		return Request{}, false
	}
	return Request{c.id, c.acked + 1, c.amounts[c.acked]}, true
}

func (c *Client) OnResponse(r Response) {
	if c.acked < len(c.amounts) && r.Client == c.id && r.Seq == c.acked+1 {
		c.acked++
	}
}

func (c *Client) Done() bool {
	return c.acked >= len(c.amounts)
}

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
