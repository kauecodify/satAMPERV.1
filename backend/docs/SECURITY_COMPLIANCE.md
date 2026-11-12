# satAMPERV.1 Backend - Security & Compliance

## Overview

This document outlines the security architecture and compliance framework for the satAMPERV.1 backend, designed to support $80M+ valuation with 10,000+ devices across 5+ countries.

## Security Architecture

### Defense in Depth Strategy

The satAMPERV.1 backend implements a **5-layer defense in depth** security model:

#### Layer 1: Device Security
- **HMAC-SHA256 Signatures**: All messages from devices are signed with HMAC-SHA256
- **Device Authentication**: JWT tokens for device authentication
- **Firmware Signing**: All firmware updates are cryptographically signed
- **Secure Boot**: ESP32 secure boot enabled
- **Flash Encryption**: ESP32 flash encryption enabled

#### Layer 2: Transport Security
- **TLS 1.3**: All communications use TLS 1.3 encryption
- **Mutual TLS**: Device connections use mutual TLS authentication
- **IP Whitelisting**: Satellite gateways have restricted IP access
- **VPN**: Internal cluster communication via VPN

#### Layer 3: Application Security
- **JWT/OAuth2**: API authentication with industry-standard protocols
- **RBAC**: Role-based access control with fine-grained permissions
- **Rate Limiting**: DDoS protection with rate limiting
- **Input Validation**: All inputs are validated and sanitized
- **CORS**: Cross-Origin Resource Sharing configured securely

#### Layer 4: Infrastructure Security
- **Network Segmentation**: VPC, subnets, and security groups
- **Firewalls**: AWS Network Firewall for network-level protection
- **IDS/IPS**: Intrusion Detection/Prevention Systems (Snort/Suricata)
- **Secret Management**: HashiCorp Vault for secrets storage and rotation

#### Layer 5: Data Security
- **Encryption at Rest**: AES-256 encryption for all stored data
- **Encryption in Transit**: TLS 1.3 for all data in transit
- **Key Rotation**: Automatic key rotation policies
- **Data Masking**: PII data is masked in logs and exports

---

## Authentication & Authorization

### Device Authentication

#### JWT Tokens for Devices
```yaml
Issuer: satAMPERV.1 Backend
Algorithm: RS256 (Asymmetric)
Expiration: 30 days (auto-refreshable)
Rotation: Every 24 hours

Claims:
  - device_id: UUID of the device
  - organization_id: Organization the device belongs to
  - permissions: List of allowed actions
  - connectivity: Allowed connectivity types (wifi, satellite, cellular)
  - firmware_version: Current firmware version
  - region: Geographic region
```

#### Token Generation Flow
```
1. Device sends authentication request with HMAC-SHA256 signature
2. Backend verifies signature using device secret
3. Backend validates device in database
4. Backend generates JWT token with appropriate claims
5. Token returned to device
6. Device uses token for subsequent requests
```

#### Token Refresh Flow
```
1. Device sends refresh request with expiring token
2. Backend verifies token is valid and not expired
3. Backend checks device is still active
4. Backend generates new token with updated claims
5. New token returned to device
6. Old token added to revocation list
```

### User Authentication

#### OAuth2/OIDC with Auth0/Keycloak
```yaml
Provider: Auth0 or Keycloak
Protocol: OAuth2/OIDC
Flow: Authorization Code + PKCE

Multi-Factor Authentication:
  - Password (Primary)
  - TOTP (Google Authenticator, Authy)
  - WebAuthn (Hardware keys, Biometrics)
  - SMS (Backup)

Token Types:
  - Access Token: JWT, 1 hour expiration
  - Refresh Token: Opaque, 30 days expiration
  - ID Token: JWT, 1 hour expiration
```

#### User Roles and Permissions
```yaml
Roles:
  viewer:
    - Read-only access to dashboards
    - View device status and telemetry
    - View alerts
    
  operator:
    - All viewer permissions
    - Manage devices (add, remove, update)
    - Configure device settings
    - Acknowledge alerts
    - View and manage firmware
    
  admin:
    - All operator permissions
    - Manage users and roles
    - Configure organizations
    - Manage governance proposals
    - View audit logs
    
  super_admin:
    - All admin permissions
    - Platform administration
    - System configuration
    - Manage all organizations
    - Emergency access

Permissions Model:
  - Role-Based Access Control (RBAC)
  - Attribute-Based Access Control (ABAC) for fine-grained control
  - Resource-level permissions (per organization, per device)
```

---

## Authorization Model

### Resource Hierarchy
```
Organization
  └── Device Group
      └── Device
          └── Telemetry Data

Permissions are inherited down the hierarchy:
- Organization admin can manage all devices in the organization
- Device group admin can manage all devices in the group
- Device admin can manage specific device
```

### Permission Examples
```yaml
# Organization-level permissions
organization:read
organization:write
organization:delete
organization:manage_users
organization:manage_devices

# Device-level permissions
device:read
device:write
device:delete
device:configure
device:update_firmware

# Data-level permissions
telemetry:read
telemetry:export
alerts:read
alerts:manage

governance:read
governance:create_proposal
governance:vote
governance:manage
```

---

## Compliance Certifications

### SOC 2 Type II

**Status**: Target for Q3 2025

**Scope**:
- Security
- Availability
- Confidentiality
- Processing Integrity
- Privacy

**Requirements**:
- ✅ Access controls and authentication
- ✅ Network security and monitoring
- ✅ Data encryption at rest and in transit
- ✅ Incident response procedures
- ✅ Backup and disaster recovery
- ✅ Vendor management
- ✅ Risk assessment
- ✅ Security awareness training
- ⏳ Independent audit (scheduled Q3 2025)

**Timeline**:
- Q1 2025: SOC 2 Type I certification
- Q3 2025: SOC 2 Type II certification
- Annual: Recertification

**Cost**: ~$100K (audit + remediation)

### ISO 27001

**Status**: Target for Q4 2025

**Scope**:
- Information Security Management System (ISMS)
- All backend infrastructure and services

**Requirements**:
- ✅ Information security policy
- ✅ Risk assessment and treatment
- ✅ Security controls implementation
- ✅ Security awareness training
- ✅ Incident management
- ✅ Business continuity
- ✅ Compliance monitoring
- ⏳ Independent audit (scheduled Q4 2025)

**Timeline**:
- Q2 2025: ISMS implementation
- Q4 2025: ISO 27001 certification
- Annual: Surveillance audits
- Triennial: Recertification

**Cost**: ~$75K (implementation + audit)

### GDPR Compliance

**Status**: Compliant (Ongoing)

**Scope**:
- All European Union operations
- Personal data processing

**Requirements**:
- ✅ Data minimization
- ✅ Purpose limitation
- ✅ Data subject rights (access, rectification, erasure, portability)
- ✅ Right to be forgotten
- ✅ Data breach notification (within 72 hours)
- ✅ Privacy by design and default
- ✅ Data Protection Impact Assessments (DPIA)
- ✅ Data Processing Agreements (DPA) with all vendors

**Data Categories**:
- Device identifiers (UUIDs)
- User account information
- Organization information
- Location data (aggregated, anonymized)
- Telemetry data (non-personal)

**Data Retention**:
- Device identifiers: Permanent (required for operation)
- User account information: Until account deletion
- Telemetry data: 13 months raw, 10 years aggregated
- Logs: 30 days hot, 1 year cold, 7 years archive

### LGPD Compliance

**Status**: Compliant (Ongoing)

**Scope**:
- All Brazilian operations
- Personal data processing in Brazil

**Requirements**:
- ✅ Lawful basis for processing
- ✅ Data subject rights
- ✅ Data breach notification
- ✅ Data Protection Officer (DPO) appointment
- ✅ Record of processing activities

**DPO**:
- Name: satAMPERV.1 Data Protection Officer
- Email: dpo@satamperv.com
- Location: Brazil

---

## Data Protection

### Data Classification

| Classification | Description | Examples | Protection Requirements |
|---------------|-------------|----------|------------------------|
| Public | Non-sensitive data | Documentation, marketing materials | None |
| Internal | Business-sensitive data | Device configurations, internal metrics | Access control, encryption at rest |
| Confidential | Sensitive business data | Customer contracts, pricing, financial data | Access control, encryption at rest and in transit, audit logging |
| Restricted | Highly sensitive data | Device secrets, API keys, certificates | Strict access control, encryption, key management, audit logging |
| PII | Personal data | User emails, names, addresses | All of the above + GDPR/LGPD compliance |

### Encryption Standards

#### Encryption at Rest
```yaml
Algorithm: AES-256
Mode: GCM (Galois/Counter Mode)
Key Size: 256 bits

Implementation:
  - AWS EBS: Encrypted by default
  - AWS S3: SSE-S3 or SSE-KMS
  - AWS RDS: Encrypted by default
  - MinIO: Server-side encryption
  - Local storage: LUKS or similar

Key Management:
  - AWS KMS: Primary key management
  - HashiCorp Vault: Secondary key management
  - Key rotation: Every 90 days
  - Key versioning: Enabled
```

#### Encryption in Transit
```yaml
Protocol: TLS 1.3
Cipher Suites:
  - TLS_AES_256_GCM_SHA384
  - TLS_CHACHA20_POLY1305_SHA256
  - TLS_AES_128_GCM_SHA256

Certificate Management:
  - Certificate Authority: AWS ACM or Let's Encrypt
  - Certificate rotation: Every 90 days
  - Certificate validation: OCSP stapling
  - Mutual TLS: For device connections
```

### Key Management

#### AWS KMS
```yaml
Key Types:
  - Customer Master Keys (CMK): Managed by AWS or customer
  - Data Keys: Generated for each encryption operation

Key Policies:
  - Least privilege access
  - Key rotation enabled
  - Key deletion disabled (for critical keys)
  - CloudTrail logging enabled

Key Usage:
  - EBS volumes
  - S3 buckets
  - RDS databases
  - Secrets encryption
```

#### HashiCorp Vault
```yaml
Deployment: Kubernetes (Helm chart)
Storage: AWS S3 or DynamoDB
HA: 3-node cluster

Secrets Engines:
  - KV v2: Generic secrets
  - Database: PostgreSQL, TimescaleDB credentials
  - AWS: IAM credentials
  - PKI: Certificate authority
  - Transit: Encryption as a service

Policies:
  - Fine-grained access control
  - Least privilege
  - Audit logging
```

---

## Network Security

### VPC Design

```yaml
CIDR: 10.0.0.0/16 (per region)

Subnets:
  - Public: 10.0.1.0/24, 10.0.2.0/24, 10.0.3.0/24
    - Load balancers
    - NAT gateways
    - Bastion hosts
  
  - Private: 10.0.10.0/24, 10.0.11.0/24, 10.0.12.0/24
    - Kubernetes worker nodes
    - RDS instances
    - ElastiCache
  
  - Database: 10.0.20.0/24, 10.0.21.0/24, 10.0.22.0/24
    - TimescaleDB
    - PostgreSQL
    - MinIO

Security Groups:
  - sg-mqtt: Ports 1883, 8883, 8083, 8084 (from 0.0.0.0/0)
  - sg-api: Ports 80, 443 (from 0.0.0.0/0)
  - sg-flink: Ports 8081, 6123 (from VPC)
  - sg-postgres: Port 5432 (from Kubernetes CIDR)
  - sg-redis: Port 6379 (from Kubernetes CIDR)
  - sg-nats: Ports 4222, 4223 (from VPC)
```

### Firewall Rules

```yaml
# AWS Network Firewall Rules

Inbound Rules:
  - MQTT: Ports 1883, 8883 (from 0.0.0.0/0)
  - HTTP/HTTPS: Ports 80, 443 (from 0.0.0.0/0)
  - Satellite: Port 4222 (from Swarm IP ranges)
  - SSH: Port 22 (from bastion host only)
  - RDP: Port 3389 (disabled)

Outbound Rules:
  - All: Any port (to 0.0.0.0/0)

Stateful Inspection:
  - Enabled for all traffic
  - Logging enabled
  - Metrics enabled
```

### DDoS Protection

```yaml
# AWS Shield Advanced
Protection Level: Advanced

DDoS Response Team: AWS DRT

Mitigation:
  - Automatic detection and mitigation
  - Manual intervention for large attacks
  - 24/7 support

WAF Rules:
  - Rate limiting: 1000 req/min per IP
  - IP reputation: Block known bad IPs
  - SQL injection: Block SQLi attempts
  - Cross-site scripting: Block XSS attempts
  - Bad bots: Block known bot traffic
```

---

## Security Monitoring

### SIEM Integration

```yaml
Provider: AWS GuardDuty + Custom SIEM

Data Sources:
  - CloudTrail: API logging
  - VPC Flow Logs: Network traffic
  - GuardDuty: Threat detection
  - Security Hub: Compliance monitoring
  - Custom: Application logs

Alerting:
  - Critical: PagerDuty (24/7)
  - High: Email + Slack
  - Medium: Email
  - Low: Daily digest
```

### Security Metrics

```yaml
# Real-time Security Metrics
- Authentication failures
- Authorization failures
- Rate limit violations
- IP blacklist hits
- TLS handshake failures
- Encryption/decryption failures
- Key rotation events
- Certificate expiration warnings

# Aggregated Security Metrics
- Successful/failed logins
- Active sessions
- API request rates
- Data access patterns
- Security events by type
```

### Incident Response

#### Incident Classification

| Severity | Description | Response Time | Resolution Time |
|----------|-------------|---------------|-----------------|
| Critical | Active security breach, data loss, system compromise | Immediate (15 min) | 4 hours |
| High | Potential security breach, unauthorized access | 1 hour | 24 hours |
| Medium | Security vulnerability, misconfiguration | 4 hours | 7 days |
| Low | Security warning, informational | 24 hours | 30 days |

#### Incident Response Process

```
1. Detection
   - Security monitoring detects anomaly
   - Alert triggered (PagerDuty, Email, Slack)
   - Incident ticket created

2. Triage
   - On-call engineer assesses incident
   - Severity classified
   - Response team activated

3. Containment
   - Isolate affected systems
   - Block malicious actors
   - Preserve evidence

4. Eradication
   - Remove malicious code/access
   - Patch vulnerabilities
   - Rotate compromised credentials

5. Recovery
   - Restore systems from backups
   - Verify system integrity
   - Monitor for recurrence

6. Post-Incident
   - Root cause analysis
   - Lessons learned
   - Process improvements
   - Documentation update
```

#### Incident Response Team

```yaml
Primary:
  - Security Engineer (24/7 on-call)
  - DevOps Engineer (24/7 on-call)
  - Engineering Manager (escalation)

Secondary:
  - CTO (critical incidents)
  - Legal (data breaches)
  - PR/Communications (public incidents)

External:
  - AWS Support (Enterprise)
  - Security Consultants (as needed)
```

---

## Security Testing

### Penetration Testing

```yaml
Frequency: Annual
Provider: Third-party security firm
Scope: All external-facing services

Methodology:
  - Black box testing
  - Grey box testing
  - Social engineering
  - Physical security (if applicable)

Deliverables:
  - Executive summary
  - Technical report
  - Risk assessment
  - Remediation recommendations
```

### Vulnerability Scanning

```yaml
Frequency: Weekly
Tools: Nessus, AWS Inspector
Scope: All infrastructure

Scan Types:
  - Network vulnerability scanning
  - Host-based vulnerability scanning
  - Container vulnerability scanning
  - Database vulnerability scanning

Remediation:
  - Critical: Immediate (24 hours)
  - High: 7 days
  - Medium: 30 days
  - Low: 90 days
```

### Code Security

```yaml
Static Analysis:
  - Tool: SonarQube, Checkmarx
  - Frequency: On every commit
  - Languages: Go, Java, Python, JavaScript

Dynamic Analysis:
  - Tool: OWASP ZAP, Burp Suite
  - Frequency: On every release
  - Scope: All APIs and web interfaces

Dependency Scanning:
  - Tool: Dependabot, Snyk
  - Frequency: Daily
  - Action: Automatic PR creation for vulnerabilities
```

---

## Security Policies

### Password Policy

```yaml
Complexity:
  - Minimum length: 12 characters
  - Maximum length: 64 characters
  - Character types: Uppercase, lowercase, numbers, special
  - No common passwords
  - No password reuse

Rotation:
  - User passwords: Every 90 days
  - Service accounts: Every 180 days
  - API keys: Every 90 days
  - Certificates: Every 90 days

Storage:
  - Never stored in plaintext
  - Hashed with bcrypt (cost factor 12)
  - Salted with unique salt per user
```

### Access Control Policy

```yaml
Principle: Least Privilege

Access Request:
  - Justification required
  - Manager approval required
  - Time-bound access (when possible)
  - Audit trail maintained

Access Review:
  - Quarterly: All user access
  - Monthly: Privileged access
  - Annual: Full access audit

Privileged Access:
  - MFA required
  - Session recording (for sensitive systems)
  - Just-in-time access (when possible)
  - Break-glass procedures for emergencies
```

### Data Access Policy

```yaml
Classification: Public, Internal, Confidential, Restricted, PII

Access Requirements:
  - Public: No authentication required
  - Internal: Valid account required
  - Confidential: Valid account + business justification
  - Restricted: Valid account + business justification + manager approval
  - PII: Valid account + business justification + legal approval

Data Handling:
  - Encryption at rest and in transit
  - Access logging
  - No local storage (without encryption)
  - Secure deletion when no longer needed
```

### Network Access Policy

```yaml
Remote Access:
  - VPN required for internal resources
  - MFA required for VPN
  - IP whitelisting for sensitive systems
  - Session timeout: 8 hours

Bastion Host:
  - SSH key authentication only
  - No password authentication
  - Session recording enabled
  - Access logging enabled

API Access:
  - API keys required
  - Rate limiting enforced
  - IP whitelisting (when possible)
  - Usage monitoring
```

---

## Compliance Monitoring

### Continuous Compliance

```yaml
Tools:
  - AWS Config: Resource configuration compliance
  - AWS Security Hub: Security best practices
  - AWS Config Rules: Custom compliance rules
  - OpenSCAP: SCAP compliance scanning
  - Chef InSpec: Infrastructure compliance testing

Checks:
  - Encryption enabled (EBS, S3, RDS)
  - Logging enabled (CloudTrail, VPC Flow Logs)
  - Access controls configured correctly
  - Network security groups configured correctly
  - Patch levels up to date
  - Backup policies configured
  - Disaster recovery configured
```

### Audit Logging

```yaml
Retention: 7 years

Log Sources:
  - CloudTrail: All AWS API calls
  - VPC Flow Logs: All network traffic
  - Application Logs: All application events
  - Authentication Logs: All login attempts
  - Authorization Logs: All access decisions
  - Security Events: All security-related events

Log Protection:
  - Immutable storage (S3 Object Lock)
  - Encryption at rest (AES-256)
  - Access controls (least privilege)
  - Integrity verification (hashing)
```

---

## Data Subject Requests

### GDPR/LGPD Requests

```yaml
Request Types:
  - Access: Request to access personal data
  - Rectification: Request to correct personal data
  - Erasure: Request to delete personal data (Right to be Forgotten)
  - Portability: Request to export personal data
  - Restriction: Request to restrict processing
  - Objection: Request to object to processing

Response Times:
  - Acknowledgment: 24 hours
  - Response: 30 days (extendable to 60 days for complex requests)

Process:
  1. Request received (email, web form, mail)
  2. Identity verification
  3. Request validation
  4. Data location identification
  5. Data retrieval/processing
  6. Response generation
  7. Response delivery
  8. Request logging
```

### Data Breach Notification

```yaml
Detection: Within 72 hours of discovery

Notification Timeline:
  - Internal: Immediate
  - Supervisory Authority: Within 72 hours
  - Data Subjects: Within 72 hours (if high risk)

Notification Content:
  - Nature of the breach
  - Categories of data affected
  - Number of data subjects affected
  - Data Protection Officer contact
  - Likely consequences
  - Measures taken or proposed
```

---

## Vendor Security

### Vendor Management

```yaml
Vendor Classification:
  - Critical: Direct access to production systems
  - High: Access to sensitive data
  - Medium: Access to internal systems
  - Low: No access to systems or data

Security Requirements:
  - Critical: SOC 2 Type II, ISO 27001, Security questionnaire
  - High: SOC 2 Type II or ISO 27001
  - Medium: Security questionnaire
  - Low: Basic security practices

Contractual Requirements:
  - Data Processing Agreement (DPA)
  - Security addendum
  - Incident notification clause
  - Audit rights
  - Termination for cause
```

### Vendor List

```yaml
Cloud Providers:
  - AWS: SOC 2 Type II, ISO 27001, PCI DSS
  - Azure: SOC 2 Type II, ISO 27001, PCI DSS
  - GCP: SOC 2 Type II, ISO 27001, PCI DSS

Satellite Provider:
  - Swarm Technologies: SOC 2 Type I, ISO 27001 (in progress)

Database:
  - TimescaleDB: SOC 2 Type II (via AWS)
  - PostgreSQL: SOC 2 Type II (via AWS)

Message Queue:
  - EMQX: SOC 2 Type II (Enterprise)
  - NATS: Open source, self-hosted

Monitoring:
  - Prometheus: Open source, self-hosted
  - Grafana: Open source, self-hosted
  - Loki: Open source, self-hosted

Container Platform:
  - Kubernetes: Open source, self-hosted
  - Docker: Open source
```

---

## Security Training

### Employee Training

```yaml
Onboarding:
  - Security awareness training
  - Acceptable use policy
  - Password policy
  - Phishing awareness

Annual Training:
  - Security best practices
  - Incident reporting
  - Data protection
  - Compliance requirements

Role-Specific Training:
  - Engineers: Secure coding, DevSecOps
  - DevOps: Infrastructure security, IAM
  - Support: Data handling, PII protection
  - Management: Security governance, risk management
```

### Phishing Testing

```yaml
Frequency: Quarterly
Provider: Third-party service
Scope: All employees

Metrics:
  - Click rate
  - Credential submission rate
  - Reporting rate
  - Time to report

Follow-up:
  - Immediate notification to clicked users
  - Additional training for vulnerable users
  - Trend analysis and improvement
```

---

## Security Metrics

### Key Security Metrics

```yaml
# Vulnerability Metrics
- Mean Time to Detect (MTTD): < 24 hours
- Mean Time to Respond (MTTR): < 4 hours
- Mean Time to Resolve (MTTR): < 7 days
- Vulnerability Remediation Rate: > 90% within SLA

# Access Metrics
- Privileged Accounts: < 10% of total accounts
- Dormant Accounts: 0 (automatic deactivation)
- Access Review Completion: 100% quarterly

# Incident Metrics
- Security Incidents: < 5 per quarter
- Critical Incidents: 0
- Data Breaches: 0

# Compliance Metrics
- Compliance Score: > 95%
- Audit Findings: < 5 per audit
- Critical Findings: 0
```

---

## Security Roadmap

### 2025 Roadmap

```yaml
Q1 2025:
  - SOC 2 Type I certification
  - Implement security monitoring (SIEM)
  - Deploy WAF and DDoS protection
  - Conduct penetration testing
  - Security awareness training

Q2 2025:
  - SOC 2 Type II certification (preparation)
  - ISO 27001 implementation
  - Deploy HashiCorp Vault
  - Implement network segmentation
  - Conduct vulnerability scanning

Q3 2025:
  - SOC 2 Type II certification
  - ISO 27001 certification (preparation)
  - Deploy security automation
  - Conduct red team exercise
  - Security training program

Q4 2025:
  - ISO 27001 certification
  - PCI DSS certification (if needed)
  - Deploy zero-trust architecture
  - Conduct security audit
  - Update security policies
```

---

## Contacts

### Security Team

```yaml
Chief Information Security Officer (CISO):
  Name: [To be appointed]
  Email: ciso@satamperv.com
  Phone: +55 11 99999-9999

Data Protection Officer (DPO):
  Name: [To be appointed]
  Email: dpo@satamperv.com
  Phone: +55 11 99999-9999

Security Engineer:
  Name: [To be appointed]
  Email: security@satamperv.com
  Phone: +55 11 99999-9999

Security Incident Response:
  Email: security-incident@satamperv.com
  Phone: +55 11 99999-9999 (24/7)
```

### External Contacts

```yaml
AWS Security:
  Email: aws-security@amazon.com
  Phone: Varies by region

AWS Support (Enterprise):
  Phone: 24/7 support line
  Account Manager: [To be assigned]

Legal Counsel:
  Firm: [To be appointed]
  Email: legal@satamperv.com
  Phone: +55 11 99999-9999

Insurance:
  Provider: [To be appointed]
  Policy: Cyber liability insurance
  Coverage: $10M
```

---

## Summary

The satAMPERV.1 backend implements a comprehensive security and compliance framework designed to:

1. **Protect**: Devices, data, and infrastructure from threats
2. **Detect**: Security incidents and vulnerabilities in real-time
3. **Respond**: To security incidents quickly and effectively
4. **Comply**: With all relevant regulations and standards
5. **Improve**: Security posture continuously

This framework supports the scaling of satAMPERV.1 to 10,000+ devices with $80M+ valuation, ensuring security and compliance across 5+ countries.
