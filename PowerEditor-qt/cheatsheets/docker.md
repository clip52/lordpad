# Docker Cheatsheet

## Containers básicos
```bash
docker run hello-world
docker run -it ubuntu bash                 # interativo
docker run --rm alpine echo hi             # remove ao sair
docker run -d --name web -p 8080:80 nginx  # daemon, port-map
docker ps                                  # rodando
docker ps -a                               # incluindo parados
docker stop web; docker start web
docker restart web
docker rm web; docker rm -f web            # forçar
docker logs -f --tail=100 web
docker exec -it web sh                     # shell em container
docker stats                               # CPU/MEM/IO ao vivo
docker inspect web | jq '.[0].State'
docker top web                             # processos
docker port web                            # mapeamentos
```

## Imagens
```bash
docker images
docker pull alpine:3.20
docker rmi alpine:old
docker tag src:latest dest:1.0
docker push myreg.io/repo:tag
docker history alpine                      # camadas
docker image prune -a                      # remove unused
docker system df                           # uso de disco
docker system prune -a --volumes           # nuke (cuidado!)
```

## Build
```bash
docker build -t myapp:1.0 .
docker build --target=builder -t myapp:dev .
docker build --build-arg KEY=val .
docker buildx build --platform linux/amd64,linux/arm64 -t app:multi --push .
docker buildx prune
```

## Dockerfile (template)
```dockerfile
# multi-stage
FROM node:20-alpine AS builder
WORKDIR /app
COPY package*.json ./
RUN npm ci --omit=dev
COPY . .
RUN npm run build

FROM node:20-alpine
WORKDIR /app
COPY --from=builder /app/dist ./dist
COPY --from=builder /app/node_modules ./node_modules
USER node
ENV NODE_ENV=production
EXPOSE 3000
HEALTHCHECK --interval=30s --timeout=3s \
    CMD wget -q -O- http://localhost:3000/health || exit 1
CMD ["node", "dist/server.js"]
```

## Volumes
```bash
docker volume create dbdata
docker volume ls
docker run -v dbdata:/var/lib/postgresql/data postgres
docker run -v "$PWD:/code" -w /code python python script.py
docker volume inspect dbdata
docker volume prune
```

## Network
```bash
docker network create app
docker network ls
docker run --network app --name db postgres
docker run --network app --name web nginx       # web → http://db
docker network inspect app
```

## docker compose
```bash
docker compose up -d                       # background
docker compose up --build                  # rebuild
docker compose down                        # para + remove
docker compose down -v                     # + volumes
docker compose ps
docker compose logs -f web
docker compose exec web sh
docker compose restart web
docker compose pull
docker compose config                      # mostra YAML resolvido
```

## docker-compose.yml exemplo
```yaml
services:
  web:
    build: .
    ports: ["8080:80"]
    environment:
      - DATABASE_URL=postgres://db:5432/app
    depends_on:
      db:
        condition: service_healthy
    healthcheck:
      test: ["CMD", "curl", "-f", "http://localhost/health"]
      interval: 30s
      timeout: 3s
      retries: 3
    restart: unless-stopped
    volumes:
      - ./logs:/var/log/app
  db:
    image: postgres:16
    environment:
      POSTGRES_PASSWORD: secret
    volumes:
      - dbdata:/var/lib/postgresql/data
    healthcheck:
      test: ["CMD-SHELL", "pg_isready -U postgres"]
      interval: 5s
      timeout: 3s
      retries: 5

volumes:
  dbdata:
```

## Bind-mount vs volume
- `-v $PWD:/app` (bind) — pasta do host, ótimo para dev
- `-v dbdata:/data` (named volume) — gerenciado pelo docker, prod
- `-v /data` (anônimo) — orphan, evite

## Boas práticas
- Use `.dockerignore` (igual `.gitignore`) — `node_modules`, `.git`, `.env`
- Camadas: COPY package.json antes do RUN install (cache!)
- USER não-root sempre que possível
- Multi-stage para imagens menores
- Tag imutável (`:1.2.3`) em prod, não `:latest`
- HEALTHCHECK em todo serviço web
- Log para stdout/stderr, deixe o orquestrador coletar
```

## Debug rápido
```bash
docker run --rm -it --entrypoint sh imagem  # entra antes do CMD
docker cp web:/etc/nginx/nginx.conf .       # copia do container
docker cp ./conf.d web:/etc/nginx/          # copia para
docker diff web                             # alterações vs imagem
```
