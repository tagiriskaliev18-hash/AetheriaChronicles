@echo off
title Aethelgard: Ashes of the Drake — Local Server
cd /d "%~dp0"
echo Starting Aethelgard on http://localhost:8080 ...
start "" "http://localhost:8080"
node -e "const http=require('http'),fs=require('fs'),path=require('path');const root=path.join(process.cwd(),'dist-desktop');const mime={'.html':'text/html; charset=utf-8','.js':'application/javascript; charset=utf-8','.css':'text/css; charset=utf-8','.png':'image/png','.svg':'image/svg+xml','.ico':'image/x-icon'};http.createServer((req,res)=>{const p=decodeURIComponent((req.url||'/').split('?')[0]);const f=path.join(root,p==='/'?'index.html':p.replace(/^\/+/,''));if(f.startsWith(root)&&fs.existsSync(f)&&fs.statSync(f).isFile()){res.writeHead(200,{'Content-Type':mime[path.extname(f)]||'application/octet-stream','Cache-Control':'no-cache'});fs.createReadStream(f).pipe(res);}else{res.writeHead(404);res.end('Not found');}}).listen(8080,()=>console.log('Running at http://localhost:8080 (close this window to stop)'));"
pause
