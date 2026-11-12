# Network Module for satAMPERV.1 Backend
variable "region" { type = string }
variable "environment" { type = string }
variable "vpc_cidr" { type = string; default = "10.0.0.0/16" }
variable "azs" { type = list(string); default = ["a", "b", "c"] }

resource "aws_vpc" "main" {
  cidr_block = var.vpc_cidr
  enable_dns_support = true
  enable_dns_hostnames = true
  tags = { Name = "satamperv-${var.environment}-${var.region}"; Environment = var.environment }
}

resource "aws_internet_gateway" "igw" {
  vpc_id = aws_vpc.main.id
  tags = { Name = "satamperv-${var.environment}-igw"; Environment = var.environment }
}

resource "aws_eip" "nat" {
  count = length(var.azs)
  tags = { Name = "satamperv-${var.environment}-nat-eip-${element(var.azs, count.index)}" }
}

resource "aws_nat_gateway" "nat" {
  count = length(var.azs)
  allocation_id = aws_eip.nat[count.index].id
  subnet_id = aws_subnet.public[count.index].id
  tags = { Name = "satamperv-${var.environment}-nat-${element(var.azs, count.index)}" }
  depends_on = [aws_internet_gateway.igw]
}

resource "aws_subnet" "public" {
  count = length(var.azs)
  vpc_id = aws_vpc.main.id
  cidr_block = cidrsubnet(var.vpc_cidr, 8, count.index + 1)
  availability_zone = "${var.region}${element(var.azs, count.index)}"
  map_public_ip_on_launch = true
  tags = { Name = "satamperv-${var.environment}-public-${element(var.azs, count.index)}"; Type = "public" }
}

resource "aws_subnet" "private" {
  count = length(var.azs)
  vpc_id = aws_vpc.main.id
  cidr_block = cidrsubnet(var.vpc_cidr, 8, count.index + 10)
  availability_zone = "${var.region}${element(var.azs, count.index)}"
  tags = { Name = "satamperv-${var.environment}-private-${element(var.azs, count.index)}"; Type = "private" }
}

resource "aws_subnet" "database" {
  count = length(var.azs)
  vpc_id = aws_vpc.main.id
  cidr_block = cidrsubnet(var.vpc_cidr, 8, count.index + 20)
  availability_zone = "${var.region}${element(var.azs, count.index)}"
  tags = { Name = "satamperv-${var.environment}-database-${element(var.azs, count.index)}"; Type = "database" }
}

resource "aws_route_table" "public" {
  vpc_id = aws_vpc.main.id
  route { cidr_block = "0.0.0.0/0"; gateway_id = aws_internet_gateway.igw.id }
  tags = { Name = "satamperv-${var.environment}-public-rt" }
}

resource "aws_route_table" "private" {
  count = length(var.azs)
  vpc_id = aws_vpc.main.id
  route { cidr_block = "0.0.0.0/0"; nat_gateway_id = aws_nat_gateway.nat[count.index].id }
  tags = { Name = "satamperv-${var.environment}-private-rt-${element(var.azs, count.index)}" }
}

resource "aws_route_table_association" "public" {
  count = length(var.azs)
  subnet_id = aws_subnet.public[count.index].id
  route_table_id = aws_route_table.public.id
}

resource "aws_route_table_association" "private" {
  count = length(var.azs)
  subnet_id = aws_subnet.private[count.index].id
  route_table_id = aws_route_table.private[count.index].id
}

resource "aws_security_group" "mqtt" {
  name = "satamperv-${var.environment}-mqtt"
  vpc_id = aws_vpc.main.id
  ingress { from_port = 1883; to_port = 1883; protocol = "tcp"; cidr_blocks = ["0.0.0.0/0"]; description = "MQTT" }
  ingress { from_port = 8883; to_port = 8883; protocol = "tcp"; cidr_blocks = ["0.0.0.0/0"]; description = "MQTT TLS" }
  ingress { from_port = 8083; to_port = 8083; protocol = "tcp"; cidr_blocks = ["0.0.0.0/0"]; description = "MQTT WS" }
  ingress { from_port = 8084; to_port = 8084; protocol = "tcp"; cidr_blocks = ["0.0.0.0/0"]; description = "MQTT WSS" }
  egress { from_port = 0; to_port = 0; protocol = "-1"; cidr_blocks = ["0.0.0.0/0"] }
}

resource "aws_security_group" "nats" {
  name = "satamperv-${var.environment}-nats"
  vpc_id = aws_vpc.main.id
  ingress { from_port = 4222; to_port = 4222; protocol = "tcp"; cidr_blocks = [var.vpc_cidr] }
  ingress { from_port = 4223; to_port = 4223; protocol = "tcp"; cidr_blocks = [var.vpc_cidr] }
  egress { from_port = 0; to_port = 0; protocol = "-1"; cidr_blocks = ["0.0.0.0/0"] }
}

resource "aws_security_group" "api" {
  name = "satamperv-${var.environment}-api"
  vpc_id = aws_vpc.main.id
  ingress { from_port = 80; to_port = 80; protocol = "tcp"; cidr_blocks = ["0.0.0.0/0"] }
  ingress { from_port = 443; to_port = 443; protocol = "tcp"; cidr_blocks = ["0.0.0.0/0"] }
  egress { from_port = 0; to_port = 0; protocol = "-1"; cidr_blocks = ["0.0.0.0/0"] }
}

resource "aws_security_group" "flink" {
  name = "satamperv-${var.environment}-flink"
  vpc_id = aws_vpc.main.id
  ingress { from_port = 8081; to_port = 8081; protocol = "tcp"; cidr_blocks = [var.vpc_cidr] }
  ingress { from_port = 6123; to_port = 6123; protocol = "tcp"; cidr_blocks = [var.vpc_cidr] }
  egress { from_port = 0; to_port = 0; protocol = "-1"; cidr_blocks = ["0.0.0.0/0"] }
}

resource "aws_security_group" "postgres" {
  name = "satamperv-${var.environment}-postgres"
  vpc_id = aws_vpc.main.id
  ingress { from_port = 5432; to_port = 5432; protocol = "tcp"; cidr_blocks = [var.vpc_cidr] }
  egress { from_port = 0; to_port = 0; protocol = "-1"; cidr_blocks = ["0.0.0.0/0"] }
}

resource "aws_security_group" "redis" {
  name = "satamperv-${var.environment}-redis"
  vpc_id = aws_vpc.main.id
  ingress { from_port = 6379; to_port = 6379; protocol = "tcp"; cidr_blocks = [var.vpc_cidr] }
  egress { from_port = 0; to_port = 0; protocol = "-1"; cidr_blocks = ["0.0.0.0/0"] }
}

output "vpc_id" { value = aws_vpc.main.id }
output "public_subnet_ids" { value = aws_subnet.public[*].id }
output "private_subnet_ids" { value = aws_subnet.private[*].id }
output "database_subnet_ids" { value = aws_subnet.database[*].id }
output "security_group_mqtt_id" { value = aws_security_group.mqtt.id }
output "security_group_nats_id" { value = aws_security_group.nats.id }
output "security_group_api_id" { value = aws_security_group.api.id }
output "security_group_flink_id" { value = aws_security_group.flink.id }
output "security_group_postgres_id" { value = aws_security_group.postgres.id }
output "security_group_redis_id" { value = aws_security_group.redis.id }
