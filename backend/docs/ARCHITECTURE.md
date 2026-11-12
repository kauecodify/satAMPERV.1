# satAMPERV.1 Backend - Detailed Architecture

## System Context (C4 Level 1)

### Overview
The satAMPERV.1 backend receives telemetry from 10,000+ ESP32 devices via Wi-Fi/MQTT (primary) and Swarm M138 satellite (fallback), processes it in real-time, stores it efficiently, and provides APIs for dashboards and governance.

### Actors
- **ESP32 Devices**: IoT devices collecting energy telemetry (voltage, current, power, temperature)
- **Swarm M138 Network**: Satellite connectivity provider for remote locations
- **Web/Mobile Dashboard**: User interface for monitoring and control
- **Utilities Partners**: Energy companies integrating with the platform
- **Admin Users**: Platform administrators managing devices and governance

### System Boundary
The satAMPERV.1 Backend includes:
- Ingestion Layer: MQTT Broker, Satellite Message Queue, API Gateway
- Processing Layer: Stream Processor (Flink), Governance Engine, Analytics Engine (Spark)
- Storage Layer: TimescaleDB, PostgreSQL, S3/MinIO
- Observability: Prometheus, Grafana, Loki, Jaeger

## Container Diagrams (C4 Level 2)

### Ingestion Layer Containers

#### MQTT Broker (EMQX)
- **Technology**: EMQX Enterprise 5.x
- **Purpose**: Handle Wi-Fi/MQTT connections from devices
- **Capacity**: 100K+ concurrent connections
- **Ports**: 1883 (MQTT), 8883 (MQTT/TLS), 8083 (WebSocket), 8084 (WebSocket/TLS)
- **Features**:
  - Multi-protocol: MQTT v3.1.1, v5.0, MQTT-SN, CoAP, STOMP
  - QoS 0, 1, 2 support
  - TLS 1.3 encryption
  - Authentication: JWT, Client Certificates, Username/Password
  - Authorization: ACL based
  - Rate limiting: 100 msg/sec/device
  - Cluster: 3-node for HA
  - Persistence: Mnesia, PostgreSQL

#### Satellite Message Queue (NATS JetStream)
- **Technology**: NATS Server 2.10+ with JetStream
- **Purpose**: Buffer and process satellite messages from Swarm M138
- **Capacity**: 1M+ messages/day
- **Ports**: 4222 (NATS), 4223 (NATS/TLS)
- **Features**:
  - High-performance messaging (10M+ msg/sec)
  - Binary CBOR payload support
  - HMAC-SHA256 signature verification
  - Message deduplication
  - Retry logic with exponential backoff
  - Dead letter queue
  - Cluster: 3-node for HA
  - Persistence: File-based, JetStream

#### API Gateway (Kong)
- **Technology**: Kong Enterprise 3.x
- **Purpose**: REST/gRPC API for external integrations
- **Ports**: 80 (HTTP), 443 (HTTPS)
- **Features**:
  - REST and gRPC support
  - JWT/OAuth2/OIDC authentication
  - Rate limiting (1000 req/min per client)
  - IP whitelisting/blacklisting
  - Request/response transformation
  - Circuit breaking
  - Caching (Redis)
  - Plugins: JWT, Rate Limiting, ACL, CORS

### Processing Layer Containers

#### Stream Processor (Apache Flink)
- **Technology**: Apache Flink 1.18+
- **Purpose**: Real-time telemetry processing
- **Ports**: 8081 (REST API), 6123 (TaskManager RPC)
- **Features**:
  - CBOR decoding and validation
  - HMAC-SHA256 signature verification
  - Device authentication (JWT validation)
  - Data enrichment (geolocation, device metadata)
  - Anomaly detection (voltage spikes, temperature thresholds)
  - Aggregation (5-min, hourly, daily)
  - Alert generation
  - Exactly-once processing semantics
  - Checkpointing every 10 seconds
- **Scaling**:
  - JobManager: 1-3 nodes (HA)
  - TaskManager: 10+ nodes
  - Parallelism: 100+ slots
  - State backend: RocksDB

#### Governance Engine
- **Technology**: Custom Go application
- **Purpose**: Distributed decision making at scale
- **Ports**: 8082 (HTTP API)
- **Features**:
  - Proposal management (CRUD)
  - Vote collection and validation
  - Consensus calculation (quorum, threshold)
  - Global decision propagation
  - Audit trail
  - Conflict resolution
  - Weighted voting based on device reliability
  - Dynamic quorum (67% critical, 51% standard)
  - Time-based vote expiration
  - Geographic distribution requirements
- **Scaling**:
  - Horizontal scaling with Kubernetes
  - Redis cluster for caching
  - PostgreSQL for persistence

#### Analytics Engine (Apache Spark)
- **Technology**: Apache Spark 3.5+
- **Purpose**: Batch processing and ML
- **Ports**: 8083 (REST API), 4040 (UI)
- **Features**:
  - Daily aggregation and reporting
  - Predictive maintenance models
  - Energy consumption forecasting
  - Anomaly detection training
  - Device health scoring
- **Scaling**:
  - Cluster: 5+ worker nodes
  - Memory: 64GB per worker
  - Storage: 10TB+ HDFS/S3
  - Schedule: Daily at 02:00 UTC

### Storage Layer Containers

#### TimescaleDB
- **Technology**: TimescaleDB 2.12+ (PostgreSQL 15+)
- **Purpose**: Telemetry data storage
- **Ports**: 5432
- **Schema**:
  ```sql
  CREATE TABLE telemetry (
    time TIMESTAMPTZ NOT NULL,
    device_id UUID NOT NULL,
    voltage FLOAT,
    current FLOAT,
    power FLOAT,
    temperature FLOAT,
    signal_strength INT,
    connectivity_type ENUM('wifi', 'satellite', 'cellular'),
    location GEOGRAPHY(POINT, 4326),
    hmac_signature BYTEA
  );
  SELECT create_hypertable('telemetry', 'time');
  ```
- **Retention**:
  - Raw: 13 months
  - 5-min aggregates: 2 years
  - Hourly aggregates: 5 years
  - Daily aggregates: 10 years
- **Partitioning**: By time (daily) and device_id (hash)
- **Compression**: Columnar (~70% reduction)

#### PostgreSQL
- **Technology**: PostgreSQL 15+
- **Purpose**: Device metadata, users, configurations, governance
- **Ports**: 5432
- **Key Tables**:
  - devices: Device information and status
  - users: User accounts and permissions
  - organizations: Customer organizations
  - governance_proposals: Governance proposals
  - governance_votes: Device votes
  - alerts: Alert history

#### S3/MinIO
- **Technology**: MinIO or AWS S3
- **Purpose**: Firmware, logs, backups, exports
- **Ports**: 9000 (MinIO), 443 (S3)
- **Buckets**:
  - firmware: Device firmware binaries
  - logs: Application and device logs
  - backups: Database backups
  - exports: Data exports
  - documents: Compliance documents
- **Retention**:
  - Firmware: Permanent
  - Logs: 30 days (hot), 1 year (cold), 7 years (archive)
  - Backups: 30 days (daily), 1 year (weekly), 7 years (monthly)
- **Encryption**: AES-256 at rest, TLS 1.3 in transit

## Data Flow Diagrams

### Telemetry Flow (Wi-Fi Path)
```
ESP32 Device -> Wi-Fi Network -> MQTT Broker (EMQX) -> Flink Processor -> TimescaleDB
```

Processing Steps:
1. **CBOR Decoding**: Extract deviceId, timestamp, voltage, current, power, temperature
2. **HMAC Verification**: Recompute and compare HMAC-SHA256 signature
3. **Device Authentication**: Validate JWT token and device status
4. **Data Enrichment**: Add device metadata, geolocation, connectivity type
5. **Anomaly Detection**: Check voltage, current, temperature thresholds
6. **Aggregation**: Compute 5-min, hourly, daily aggregates
7. **Alert Generation**: Create alerts for anomalies
8. **Sink**: Store raw data and aggregates in TimescaleDB

### Telemetry Flow (Satellite Path)
```
ESP32 Device -> Swarm M138 Modem -> NATS JetStream -> Flink Processor -> TimescaleDB
```

Additional Processing:
- Priority processing for satellite messages
- Retry logic for failed messages
- Dead letter queue for unprocessable messages
- Higher priority in Flink processing

### Governance Flow
```
1. Admin creates proposal via HTTP API
2. Proposal stored in PostgreSQL and published to MQTT
3. Devices receive proposal via MQTT
4. Devices evaluate locally and emit signed votes
5. Votes collected via HTTP API or MQTT
6. Governance Engine validates votes and calculates consensus
7. Decision published to MQTT and stored in PostgreSQL
8. All devices receive decision and update local state
```

### Alert Flow
```
1. Flink detects anomaly in telemetry
2. Alert Service receives alert from Flink
3. Alert deduplicated and enriched
4. Alert stored in PostgreSQL
5. Alert routed to notification channels:
   - Email (SMTP)
   - SMS (Twilio)
   - Webhook (Customer systems)
   - MQTT (Real-time dashboard updates)
```

## Deployment Diagrams

### Multi-Region Deployment

**Regions:**
- **Primary**: sa-east-1 (Brazil) - COP31 host
- **Secondary**: us-east-1 (USA), eu-central-1 (Europe)
- **Edge**: ap-southeast-1 (Asia)

**Per-Region Services:**
- Kubernetes Cluster (EKS)
- MQTT Broker (EMQX) - 3 nodes
- NATS JetStream - 3 nodes
- Apache Flink - JobManager (2) + TaskManager (10+)
- TimescaleDB - Primary + Replica
- PostgreSQL - Primary + Replica
- MinIO - 4 nodes (distributed)
- Monitoring Stack (Prometheus, Grafana, Loki, Jaeger)

**Global Services:**
- CDN (CloudFront/Fastly)
- DNS (Route53/AWS Global Accelerator)
- S3 (Cross-Region Replication)
- Centralized Monitoring (Aggregated metrics from all regions)

### Kubernetes Layout (Per Region)

**Namespaces:**
- **production**: All production workloads
- **monitoring**: Prometheus, Grafana, AlertManager, Loki, Jaeger
- **ingress**: Traefik, cert-manager
- **database**: TimescaleDB Operator, PostgreSQL Operator, MinIO Operator
- **message-queue**: EMQX Operator, NATS Operator
- **processing**: Flink Operator, Spark Operator

**Production Namespace Pods:**
- mqtt-broker: 3 pods (EMQX cluster)
- nats-server: 3 pods (NATS cluster)
- flink-jobmanager: 2 pods (HA)
- flink-taskmanager: 10+ pods
- governance-engine: 3 pods
- api-gateway: 3 pods (Kong)
- timescaledb: 2 pods (primary + replica)
- postgres: 2 pods (primary + replica)
- minio: 4 pods (distributed)

## Network Architecture

### VPC Design (Per Region)

**CIDR**: 10.0.0.0/16

**Subnets:**
- **Public**: 10.0.1.0/24, 10.0.2.0/24, 10.0.3.0/24
  - Load Balancers (ALB/NLB)
  - NAT Gateways
  - Bastion Hosts
- **Private**: 10.0.10.0/24, 10.0.11.0/24, 10.0.12.0/24
  - EKS Worker Nodes
  - RDS Instances
  - ElastiCache (Redis)
- **Database**: 10.0.20.0/24, 10.0.21.0/24, 10.0.22.0/24
  - TimescaleDB
  - PostgreSQL
  - MinIO

**Security Groups:**
- sg-mqtt: Ports 1883, 8883, 8083, 8084 (from 0.0.0.0/0)
- sg-nats: Ports 4222, 4223 (from VPC)
- sg-api: Ports 80, 443 (from 0.0.0.0/0)
- sg-flink: Ports 8081, 6123 (from VPC)
- sg-postgres: Port 5432 (from Kubernetes CIDR)
- sg-redis: Port 6379 (from Kubernetes CIDR)

**VPC Peering:**
- sa-east-1 <-> us-east-1
- sa-east-1 <-> eu-central-1
- us-east-1 <-> eu-central-1

## Security Architecture

### Defense in Depth

**Layer 1: Device Security**
- HMAC-SHA256 signature on all messages
- Device authentication (JWT tokens)
- Firmware signing and verification
- Secure boot and flash encryption (ESP32)

**Layer 2: Transport Security**
- TLS 1.3 for all communications
- Mutual TLS for device connections
- IP whitelisting for satellite gateways
- VPN for internal cluster communication

**Layer 3: Application Security**
- JWT/OAuth2 for API authentication
- Role-based access control (RBAC)
- Rate limiting and DDoS protection
- Input validation and sanitization

**Layer 4: Infrastructure Security**
- Network segmentation (VPC, subnets, security groups)
- Firewalls (AWS Network Firewall)
- IDS/IPS (Snort/Suricata)
- Secret management (HashiCorp Vault)

**Layer 5: Data Security**
- Encryption at rest (AES-256)
- Encryption in transit (TLS 1.3)
- Key rotation policies
- Data masking for PII

### Authentication & Authorization

**Device Authentication:**
- Method: JWT Tokens
- Issuer: satAMPERV.1 Backend
- Algorithm: RS256
- Expiration: 30 days (refreshable)
- Claims: device_id, organization_id, permissions, connectivity
- Rotation: Automatic every 24 hours

**User Authentication:**
- Method: OAuth2/OIDC
- Provider: Auth0 or Keycloak
- Factors: Password, TOTP, WebAuthn
- Roles: viewer, operator, admin, super_admin

## Disaster Recovery

### RTO/RPO Targets

| Service | RTO | RPO | Strategy |
|---------|-----|-----|----------|
| MQTT Broker | 5 min | 0 | Multi-node cluster |
| NATS Server | 5 min | 0 | Multi-node cluster |
| TimescaleDB | 15 min | 5 min | Cross-region replication |
| PostgreSQL | 15 min | 5 min | Cross-region replication |
| MinIO | 15 min | 0 | Cross-region replication |
| Flink | 10 min | 10 min | Checkpoint + savepoint |
| Governance Engine | 10 min | 5 min | Stateless + DB replication |
| API Gateway | 5 min | 0 | Multi-node cluster |

### Backup Strategy

| Data | Frequency | Retention | Storage |
|------|-----------|-----------|---------|
| TimescaleDB | Hourly | 30 days | S3 |
| PostgreSQL | Daily | 30 days | S3 |
| MinIO | Daily | 30 days | Cross-region |
| Full System | Weekly | 1 year | S3 Glacier |
| Configuration | On change | Permanent | Git + S3 |

### Failover Procedures

**Single Node Failure:**
1. Kubernetes detects node failure
2. Pods rescheduled to other nodes
3. Services continue operating
4. RTO: < 5 minutes

**Availability Zone Failure:**
1. Kubernetes detects AZ failure
2. Pods rescheduled to other AZs
3. Database replicas promote to primary
4. RTO: < 15 minutes

**Region Failure:**
1. Traffic fails over to secondary region
2. Database replicas promote to primary
3. DNS updates to point to secondary region
4. RTO: < 30 minutes

## Performance Targets

| Metric | Target | Measurement |
|--------|--------|-------------|
| MQTT Publish Latency | < 100ms | p99 |
| Satellite Message Processing | < 500ms | p99 |
| API Response Time | < 200ms | p99 |
| Database Write Throughput | 10K+ inserts/sec | Average |
| Database Read Throughput | 50K+ queries/sec | Average |
| System Availability | 99.95% | Monthly |
| Message Loss Rate | < 0.01% | Monthly |

## Summary

This architecture provides a comprehensive foundation for scaling satAMPERV.1 to 10,000+ devices with $80M-$200M valuation. It supports:
- Multi-region deployment for global reach
- Hybrid connectivity (Wi-Fi + Satellite)
- Real-time processing with Apache Flink
- Distributed governance at scale
- High availability and disaster recovery
- SOC 2, ISO 27001, GDPR, LGPD compliance
- COP31 readiness
