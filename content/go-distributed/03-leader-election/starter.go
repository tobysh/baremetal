package main

import "fmt"

type Role int

const (
	Follower Role = iota
	Candidate
	Leader
)

type VoteReq struct {
	Term, Candidate     int
	LastIndex, LastTerm int
}

type VoteResp struct {
	Term, From int
	Granted    bool
}

type Node struct {
	ID, N               int
	Term                int
	VotedFor            int
	Role                Role
	LastIndex, LastTerm int
	// add any extra state you need here
}

func NewNode(id, n int) *Node {
	return &Node{ID: id, N: n, VotedFor: -1}
}

// StartElection is called when the election timer fires.
func (n *Node) StartElection() VoteReq {
	return VoteReq{}
}

func (n *Node) HandleVoteReq(r VoteReq) VoteResp {
	return VoteResp{}
}

func (n *Node) HandleVoteResp(r VoteResp) {}

func (n *Node) HandleHeartbeat(term, leader int) bool {
	return false
}

// Try it: node 0 times out in a healthy 3-node cluster.
func main() {
	nodes := []*Node{NewNode(0, 3), NewNode(1, 3), NewNode(2, 3)}
	req := nodes[0].StartElection()
	for _, o := range nodes[1:] {
		nodes[0].HandleVoteResp(o.HandleVoteReq(req))
	}
	fmt.Println("node 0 role:", nodes[0].Role, "term:", nodes[0].Term) // role 2 = Leader
}
