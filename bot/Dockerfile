FROM node:24-slim

WORKDIR /app

# Install deps first for better layer caching.
COPY package.json package-lock.json ./
RUN npm ci --omit=dev

COPY index.js ./

ENV NODE_ENV=production
# Documentation only; actual port comes from PORT in the env file.
EXPOSE 2137

# Refresh token / sentry persist here — mount it as a volume.
VOLUME ["/app/steam-data"]

HEALTHCHECK --interval=30s --timeout=5s --start-period=20s --retries=3 \
  CMD node -e "require('http').get('http://127.0.0.1:'+(process.env.PORT||2137)+'/',r=>process.exit(r.statusCode===200?0:1)).on('error',()=>process.exit(1))"

CMD ["node", "index.js"]
