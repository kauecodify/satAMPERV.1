package main

import (
	"context"
	"encoding/json"
	"fmt"
	"log"
	"net/http"
	"os"
	"os/signal"
	"sync"
	"syscall"
	"time"

	"github.com/gin-gonic/gin"
	"github.com/google/uuid"
)

type Config struct {
	ServerPort         string
	DatabaseURL        string
	RedisURL           string
	MQTTBroker         string
	MQTTClientID       string
	JWTSecret          string
	ConsensusThreshold float64
	VoteExpiration     time.Duration
	ProposalExpiration time.Duration
	MinQuorum          float64
	MinRegions         int
}

type ProposalStatus string

const (
	ProposalStatusDraft     ProposalStatus = "draft"
	ProposalStatusActive    ProposalStatus = "active"
	ProposalStatusCompleted ProposalStatus = "completed"
	ProposalStatusExpired   ProposalStatus = "expired"
)

type Proposal struct {
	ID          uuid.UUID
	Title       string
	Description string
	Status      ProposalStatus
	Options     []string
	CreatedBy   uuid.UUID
	CreatedAt   time.Time
	ExpiresAt   time.Time
	Quorum      float64
}

type Vote struct {
	ID         uuid.UUID
	ProposalID uuid.UUID
	DeviceID   uuid.UUID
	Option     int
	Signature  string
	Timestamp  time.Time
	Region     string
}

type Decision struct {
	ID         uuid.UUID
	ProposalID uuid.UUID
	Option     int
	Consensus  float64
	DecidedAt  time.Time
}

type GovernanceEngine struct {
	config    Config
	proposals map[uuid.UUID]*Proposal
	votes     map[uuid.UUID][]*Vote
	mu        sync.RWMutex
}

func NewGovernanceEngine(config Config) *GovernanceEngine {
	return &GovernanceEngine{
		config:    config,
		proposals: make(map[uuid.UUID]*Proposal),
		votes:     make(map[uuid.UUID][]*Vote),
	}
}

func (g *GovernanceEngine) Start() error {
	router := gin.Default()
	g.registerRoutes(router)

	srv := &http.Server{
		Addr:    ":" + g.config.ServerPort,
		Handler: router,
	}

	go func() {
		if err := srv.ListenAndServe(); err != nil && err != http.ErrServerClosed {
			log.Printf("HTTP server error: %v", err)
		}
	}()

	quit := make(chan os.Signal, 1)
	signal.Notify(quit, syscall.SIGINT, syscall.SIGTERM)
	<-quit

	ctx, cancel := context.WithTimeout(context.Background(), 30*time.Second)
	defer cancel()

	if err := srv.Shutdown(ctx); err != nil {
		log.Printf("HTTP server shutdown error: %v", err)
	}

	return nil
}

func (g *GovernanceEngine) registerRoutes(router *gin.Engine) {
	router.GET("/health", g.healthCheck)

	api := router.Group("/api/v1")
	{
		proposals := api.Group("/proposals")
		{
			proposals.POST("", g.createProposal)
			proposals.GET("", g.listProposals)
			proposals.GET("/:id", g.getProposal)
			proposals.POST("/:id/activate", g.activateProposal)
		}

		votes := api.Group("/votes")
		{
			votes.POST("", g.castVote)
			votes.GET("/:proposal_id", g.listVotes)
		}

		decisions := api.Group("/decisions")
		{
			decisions.GET("", g.listDecisions)
		}
	}
}

func (g *GovernanceEngine) healthCheck(c *gin.Context) {
	c.JSON(http.StatusOK, gin.H{
		"status":    "healthy",
		"timestamp": time.Now().UTC(),
		"proposals": len(g.proposals),
		"votes":     g.countTotalVotes(),
	})
}

func (g *GovernanceEngine) createProposal(c *gin.Context) {
	var req struct {
		Title       string   `json:"title"`
		Description string   `json:"description"`
		Options     []string `json:"options"`
		Quorum      float64  `json:"quorum"`
		ExpiresIn   string   `json:"expires_in"`
	}

	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
		return
	}

	if req.Title == "" {
		c.JSON(http.StatusBadRequest, gin.H{"error": "title is required"})
		return
	}

	if len(req.Options) < 2 {
		c.JSON(http.StatusBadRequest, gin.H{"error": "at least 2 options are required"})
		return
	}

	if req.Quorum <= 0 || req.Quorum > 1 {
		c.JSON(http.StatusBadRequest, gin.H{"error": "quorum must be between 0 and 1"})
		return
	}

	expiresIn, err := time.ParseDuration(req.ExpiresIn)
	if err != nil {
		expiresIn = 7 * 24 * time.Hour
	}

	proposal := &Proposal{
		ID:          uuid.New(),
		Title:       req.Title,
		Description: req.Description,
		Status:      ProposalStatusDraft,
		Options:     req.Options,
		CreatedAt:   time.Now().UTC(),
		ExpiresAt:   time.Now().UTC().Add(expiresIn),
		Quorum:      req.Quorum,
	}

	g.mu.Lock()
	g.proposals[proposal.ID] = proposal
	g.mu.Unlock()

	c.JSON(http.StatusCreated, proposal)
}

func (g *GovernanceEngine) castVote(c *gin.Context) {
	var req struct {
		ProposalID uuid.UUID `json:"proposal_id"`
		DeviceID   uuid.UUID `json:"device_id"`
		Option     int        `json:"option"`
		Signature  string     `json:"signature"`
		Region     string     `json:"region"`
	}

	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
		return
	}

	if req.ProposalID == uuid.Nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": "proposal_id is required"})
		return
	}

	if req.DeviceID == uuid.Nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": "device_id is required"})
		return
	}

	if req.Signature == "" {
		c.JSON(http.StatusBadRequest, gin.H{"error": "signature is required"})
		return
	}

	g.mu.RLock()
	proposal, exists := g.proposals[req.ProposalID]
	g.mu.RUnlock()

	if !exists {
		c.JSON(http.StatusNotFound, gin.H{"error": "proposal not found"})
		return
	}

	if proposal.Status != ProposalStatusActive {
		c.JSON(http.StatusBadRequest, gin.H{"error": "proposal is not active"})
		return
	}

	if time.Now().UTC().After(proposal.ExpiresAt) {
		c.JSON(http.StatusBadRequest, gin.H{"error": "proposal has expired"})
		return
	}

	if g.hasDeviceVoted(req.ProposalID, req.DeviceID) {
		c.JSON(http.StatusConflict, gin.H{"error": "device has already voted"})
		return
	}

	vote := &Vote{
		ID:         uuid.New(),
		ProposalID: req.ProposalID,
		DeviceID:   req.DeviceID,
		Option:     req.Option,
		Signature:  req.Signature,
		Timestamp:  time.Now().UTC(),
		Region:     req.Region,
	}

	g.mu.Lock()
	g.votes[req.ProposalID] = append(g.votes[req.ProposalID], vote)
	g.mu.Unlock()

	c.JSON(http.StatusCreated, vote)
}

func (g *GovernanceEngine) hasDeviceVoted(proposalID, deviceID uuid.UUID) bool {
	g.mu.RLock()
	defer g.mu.RUnlock()

	for _, v := range g.votes[proposalID] {
		if v.DeviceID == deviceID {
			return true
		}
	}
	return false
}

func (g *GovernanceEngine) countTotalVotes() int {
	g.mu.RLock()
	defer g.mu.RUnlock()

	count := 0
	for _, votes := range g.votes {
		count += len(votes)
	}
	return count
}

func main() {
	config := Config{
		ServerPort: "8082",
	}

	engine := NewGovernanceEngine(config)

	log.Printf("Starting satAMPERV.1 Governance Engine on port %s", config.ServerPort)

	if err := engine.Start(); err != nil {
		log.Fatalf("Governance engine stopped: %v", err)
	}
}
