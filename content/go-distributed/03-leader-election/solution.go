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
	votes               map[int]bool
}

func NewNode(id, n int) *Node {
	return &Node{ID: id, N: n, VotedFor: -1}
}

func (n *Node) majority() int { return n.N/2 + 1 }

func (n *Node) becomeFollower(term int) {
	n.Term = term
	n.VotedFor = -1
	n.Role = Follower
	n.votes = nil
}

func (n *Node) StartElection() VoteReq {
	n.Term++
	n.Role = Candidate
	n.VotedFor = n.ID
	n.votes = map[int]bool{n.ID: true}
	if len(n.votes) >= n.majority() {
		n.Role = Leader
	}
	return VoteReq{n.Term, n.ID, n.LastIndex, n.LastTerm}
}

func (n *Node) HandleVoteReq(r VoteReq) VoteResp {
	if r.Term > n.Term {
		n.becomeFollower(r.Term)
	}
	upToDate := r.LastTerm > n.LastTerm || (r.LastTerm == n.LastTerm && r.LastIndex >= n.LastIndex)
	grant := r.Term == n.Term && (n.VotedFor == -1 || n.VotedFor == r.Candidate) && upToDate
	if grant {
		n.VotedFor = r.Candidate
	}
	return VoteResp{Term: n.Term, From: n.ID, Granted: grant}
}

func (n *Node) HandleVoteResp(r VoteResp) {
	if r.Term > n.Term {
		n.becomeFollower(r.Term)
		return
	}
	if n.Role != Candidate || r.Term != n.Term || !r.Granted {
		return
	}
	n.votes[r.From] = true
	if len(n.votes) >= n.majority() {
		n.Role = Leader
	}
}

func (n *Node) HandleHeartbeat(term, leader int) bool {
	if term < n.Term {
		return false
	}
	if term > n.Term {
		n.becomeFollower(term)
	}
	n.Role = Follower
	return true
}

func main() {
	nodes := []*Node{NewNode(0, 3), NewNode(1, 3), NewNode(2, 3)}
	req := nodes[0].StartElection()
	for _, o := range nodes[1:] {
		nodes[0].HandleVoteResp(o.HandleVoteReq(req))
	}
	fmt.Println("node 0 role:", nodes[0].Role, "term:", nodes[0].Term) // role 2 = Leader
}
