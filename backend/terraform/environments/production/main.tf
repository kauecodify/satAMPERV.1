terraform {
  required_version = ">= 1.5.0"
  required_providers {
    aws = { source = "hashicorp/aws"; version = ">= 5.0" }
  }
  backend "s3" {
    bucket = "satamperv-terraform-state"
    key = "production/terraform.tfstate"
    region = "sa-east-1"
    encrypt = true
    dynamodb_table = "satamperv-terraform-lock"
  }
}

module "network_primary" {
  source = "../../modules/network"
  region = "sa-east-1"
  environment = "production"
  vpc_cidr = "10.0.0.0/16"
  azs = ["a", "b", "c"]
}

module "network_secondary_us" {
  source = "../../modules/network"
  region = "us-east-1"
  environment = "production"
  vpc_cidr = "10.1.0.0/16"
  azs = ["a", "b", "c"]
}

module "network_tertiary_eu" {
  source = "../../modules/network"
  region = "eu-central-1"
  environment = "production"
  vpc_cidr = "10.2.0.0/16"
  azs = ["a", "b", "c"]
}

module "eks_primary" {
  source = "terraform-aws-modules/eks/aws"
  version = "~> 19.0"
  cluster_name = "satamperv-production-sa"
  cluster_version = "1.28"
  vpc_id = module.network_primary.vpc_id
  subnet_ids = module.network_primary.private_subnet_ids
  node_groups = {
    general = {
      desired_capacity = 5; max_capacity = 20; min_capacity = 3
      instance_types = ["m5.xlarge"]
      labels = { role = "general" }
    }
    database = {
      desired_capacity = 3; max_capacity = 10; min_capacity = 2
      instance_types = ["r5.xlarge"]
      labels = { role = "database" }
    }
    processing = {
      desired_capacity = 5; max_capacity = 20; min_capacity = 3
      instance_types = ["m5.2xlarge"]
      labels = { role = "processing" }
    }
  }
}

module "postgres_primary" {
  source = "terraform-aws-modules/rds/aws"
  version = "~> 6.0"
  identifier = "satamperv-postgres-primary"
  engine = "postgres"; engine_version = "15.4"
  instance_class = "db.r5.xlarge"
  allocated_storage = 100
  db_name = "satamperv"; username = "satamperv"
  password = random_password.postgres_primary.result
  port = 5432
  vpc_security_group_ids = [module.network_primary.security_group_postgres_id]
  db_subnet_group_name = aws_db_subnet_group.primary.name
  backup_retention_period = 30
}

module "timescaledb_primary" {
  source = "terraform-aws-modules/rds/aws"
  version = "~> 6.0"
  identifier = "satamperv-timescaledb-primary"
  engine = "timescaledb"; engine_version = "2.12.0"
  instance_class = "db.r5.2xlarge"
  allocated_storage = 500
  db_name = "telemetry"; username = "timescale"
  password = random_password.timescaledb_primary.result
  port = 5432
  vpc_security_group_ids = [module.network_primary.security_group_postgres_id]
  db_subnet_group_name = aws_db_subnet_group.primary.name
  backup_retention_period = 30
}

resource "aws_s3_bucket" "global" {
  bucket = "satamperv-global-${data.aws_caller_identity.current.account_id}"
  versioning { enabled = true }
  server_side_encryption_configuration {
    rule { apply_server_side_encryption_by_default { sse_algorithm = "AES256" } }
  }
}

resource "aws_cloudfront_distribution" "cdn" {
  enabled = true; is_ipv6_enabled = true
  origin {
    domain_name = module.eks_primary.cluster_endpoint
    origin_id = "eks-primary"
    custom_origin_config {
      http_port = 80; https_port = 443
      origin_protocol_policy = "https-only"
      origin_ssl_protocols = ["TLSv1.2"]
    }
  }
  default_cache_behavior {
    allowed_methods = ["GET", "HEAD", "OPTIONS", "PUT", "POST", "PATCH", "DELETE"]
    cached_methods = ["GET", "HEAD"]
    target_origin_id = "eks-primary"
    forwarded_values { query_string = true; cookies { forward = "all" } }
    viewer_protocol_policy = "redirect-to-https"
    min_ttl = 0; default_ttl = 3600; max_ttl = 86400
  }
  restrictions { geo_restriction { restriction_type = "none" } }
  viewer_certificate { cloudfront_default_certificate = true }
}

resource "random_password" "postgres_primary" {
  length = 32; special = true
  override_special = "!#$%&*()-_=+[]{}<>:?"
}

resource "random_password" "timescaledb_primary" {
  length = 32; special = true
  override_special = "!#$%&*()-_=+[]{}<>:?"
}

data "aws_caller_identity" "current" {}

resource "aws_db_subnet_group" "primary" {
  name = "satamperv-db-subnet-group-primary"
  subnet_ids = module.network_primary.database_subnet_ids
}

output "eks_primary_cluster_name" { value = module.eks_primary.cluster_name }
output "eks_primary_endpoint" { value = module.eks_primary.cluster_endpoint }
output "postgres_primary_endpoint" { value = module.postgres_primary.db_instance_endpoint }
output "timescaledb_primary_endpoint" { value = module.timescaledb_primary.db_instance_endpoint }
