(() => {
  const D = window.ROSEF_DATA;
  const $ = (id) => document.getElementById(id);
  const stage = $('stage');
  const stageClock = $('stageClock');
  const chapterTrack = $('chapterTrack');
  const playBtn = $('playBtn');
  const playIcon = $('playIcon');
  const prevBtn = $('prevBtn');
  const nextBtn = $('nextBtn');
  const restartBtn = $('restartBtn');
  const loopBtn = $('loopBtn');
  const speedSelect = $('speedSelect');
  const progressTrack = $('progressTrack');
  const progress = $('progress');
  const progressThumb = $('progressThumb');
  const reelTime = $('reelTime');
  const sceneContext = {
    index: $('infoIndex'), kicker: $('infoKicker'), title: $('infoTitle'), copy: $('infoCopy'),
    dataset: $('factDataset'), cnn: $('factCnn'), cnnSub: $('factCnnSub'), hnp: $('factHnp'), status: $('factStatus'), statusSub: $('factStatusSub')
  };
  const paperStages = [...document.querySelectorAll('.paper-stage')];
  const sourcesBtn = $('sourcesBtn');
  const sourceNoteBtn = $('sourceNoteBtn');
  const paperPageBtn = $('paperPageBtn');
  const dialog = $('sourceDialog');
  const dialogBody = $('dialogBody');
  const dialogClose = $('dialogClose');
  const fullscreenBtn = $('fullscreenBtn');

  const stages = Array.isArray(D.scenes) ? D.scenes : [];
  const reelDuration = stages.reduce((sum, scene) => sum + Number(scene.duration || 0), 0);
  let sceneIndex = 0;
  let playing = true;
  let loop = true;
  let speed = 1;
  let elapsedMs = 0;
  let startedAt = performance.now();
  let pausedAt = 0;
  let durationMs = stages.length ? Number(stages[0].duration || 0) * 1000 / speed : 0;
  let scrubbing = false;

  const esc = (value) => String(value).replace(/[&<>\"]/g, (c) => ({ '&':'&amp;', '<':'&lt;', '>':'&gt;', '\"':'&quot;' }[c] || c));
  const fmt = (seconds) => {
    const whole = Math.max(0, Math.floor(seconds));
    return `${String(Math.floor(whole / 60)).padStart(2,'0')}:${String(whole % 60).padStart(2,'0')}`;
  };
  const clamp = (n,a,b) => Math.max(a, Math.min(b,n));
  const paperStageFor = (id) => ({idea:'E1', ecdsa:'E1', dataset:'E1', leakage:'E2', cnn:'E3', inference:'E3', hnp:'E4', lll:'E4', validation:'E5'})[id];

  function svgEl(viewBox, aria, inner) {
    return `<svg viewBox="${viewBox}" role="img" aria-label="${esc(aria)}">${inner}</svg>`;
  }

  function tracePath(values, width, height, padX=18, padY=18, minV=null, maxV=null) {
    const min = minV ?? Math.min(...values);
    const max = maxV ?? Math.max(...values);
    const span = (max - min) || 1;
    return values.map((v,i) => {
      const x = padX + (i/(values.length-1)) * (width - padX*2);
      const y = padY + (1 - ((v-min)/span)) * (height-padY*2);
      return `${i ? 'L' : 'M'}${x.toFixed(2)} ${y.toFixed(2)}`;
    }).join(' ');
  }

  function microTraceSvg(seed, samples=256) {
    const vals = D.meta.trace.slice(0, samples).map((v,i) => {
      const offset = Math.sin((i + seed * 7) * .16) * .07;
      return v + offset;
    });
    return svgEl(`0 0 300 10`, 'micro waveform', `<path d="${tracePath(vals,300,10,1,1)}"/>`);
  }

  function sceneShell(className, inner) {
    return `<div class="scene ${className}">${inner}</div>`;
  }

  function renderIdea() {
    const steps = [
      ['01','ECDSA','nonce k'], ['02','LEAKAGE','HW / HD + noise'], ['03','CNN 1D','12 bit positions'], ['04','HNP','40 constraints'], ['05','LLL','candidate + verify']
    ];
    return sceneShell('scene--idea', `
      <div class="idea-canvas">
        <div class="idea-question">
          <div class="scene-header"><div><div class="scene-code">01 · MOTIVAȚIE</div></div><div class="scene-source">PAPER · INTRODUCTION</div></div>
          <p class="idea-quote">Nu urmărim doar cheia. Urmărim <em>cum</em> informația se transformă.</p>
          <p class="idea-small">Întrebarea experimentală este dacă un semnal sintetic, inspirat de leakage-ul de putere, poate alimenta un lanț coerent: semnal → clasificare de biți → HNP → LLL.</p>
        </div>
        <div class="chain-map" aria-label="Lanțul experimentului">
            ${steps.map((s,i)=>`<div class="chain-step ${i===0?'is-live':''}" style="--delay:${i*120}ms"><span class="chain-dot">${s[0]}</span><div><strong>${s[1]}</strong><small>${s[2]}</small></div></div>`).join('')}
        </div>
      </div>
      <div class="scene-note"><span>INPUT → REPRESENTATION → CONSTRAINT</span><strong>hardware este delimitat: firmware există, traces fizice nu sunt încă revendicate.</strong></div>
    `);
  }

  function renderEcdsa() {
    const graph = svgEl('0 0 900 480', 'secp256k1 stilizat: generator, nonce multiplication și semnătură', `
      <defs>
        <path id="ecdsa-route" d="M142 371 C245 330 285 288 408 244 C490 215 569 215 673 266"/>
      </defs>
      <g class="curve-grid">
        <path d="M48 96H852M48 176H852M48 256H852M48 336H852M48 416H852"/>
        <path d="M150 55V440M255 55V440M360 55V440M465 55V440M570 55V440M675 55V440M780 55V440"/>
      </g>
      <path class="curve-axis" d="M48 256H852M450 55V440"/>
      <path class="curve-main" d="M68 420 C150 382 172 289 250 271 C330 253 334 118 442 106 C526 97 553 165 614 207 C676 250 721 196 782 275 C818 322 826 380 844 420"/>
      <use href="#ecdsa-route" class="curve-route"/>
      <circle class="curve-point" cx="142" cy="371" r="8"/>
      <circle class="curve-point" cx="408" cy="244" r="8"/>
      <circle class="curve-point" cx="673" cy="266" r="8"/>
      <circle class="curve-traveller" r="5">
        <animateMotion dur="4.8s" repeatCount="indefinite" rotate="auto" keyPoints="0;1" keyTimes="0;1">
          <mpath href="#ecdsa-route"/>
        </animateMotion>
        <animate attributeName="opacity" values="0;1;1;0;0" keyTimes="0;.08;.70;.92;1" dur="4.8s" repeatCount="indefinite"/>
      </circle>
    `);
    return sceneShell('scene--ecdsa', `
      <div class="ecdsa-layout">
        <div class="ecdsa-formula">
          <div class="scene-header"><div><div class="scene-code">02 · ECDSA</div><div class="scene-title">Secretul rămâne ascuns în <em>ecuație</em>.</div></div><div class="scene-source">REPO · dataset_generator3.py<br>PAPER · §3.1</div></div>
          <div class="formula-stack">
            <div class="formula-row"><span>INPUT</span><strong>z · d · k</strong></div>
            <div class="formula-row is-main"><span>SIGN</span><strong>s = k⁻¹(z + rd) mod n</strong></div>
            <div class="formula-row"><span>POINT</span><strong>R = kG → r</strong></div>
            <div class="formula-row"><span>PUBLIC</span><strong>r · s · z</strong></div>
          </div>
        </div>
        <div class="ecdsa-graph">
          ${graph}
        </div>
      </div>
      <div class="scene-note"><span>ECDSA / SECP256K1</span><strong>Acest pas construiește mecanismul criptografic. Leakage-ul apare abia după aceea.</strong></div>
    `);
  }

  function renderDataset() {
    const makeWave = (seed, y, opacity='.28', cls='') => {
      const vals = D.meta.trace.slice(0, 256).map((v,i) => v + Math.sin((i + seed * 11) * .17) * .06 + Math.sin((i + seed) * .043) * .045);
      return `<path class="dataset-wave ${cls}" d="${tracePath(vals, 760, 44, 4, 5)}" transform="translate(0 ${y})" style="--wave-opacity:${opacity}"/>`;
    };
    const train = Array.from({length:7},(_,i)=>makeWave(i, i*38, '.24'));
    const validation = Array.from({length:3},(_,i)=>makeWave(i+20, i*38, '.36', 'is-validation'));
    const attack = Array.from({length:4},(_,i)=>makeWave(i+40, i*38, '.62', 'is-attack'));
    return sceneShell('scene--dataset', `
      <div class="dataset-layout">
        <div class="dataset-copy">
          <div class="scene-header"><div><div class="scene-code">03 · DATASET</div><div class="scene-title">Un benchmark care ține <em>experimentul</em> închis.</div></div><div class="scene-source">REPO · artifacts/profile_dataset.json<br>PROTOCOL · §split</div></div>
          <div class="dataset-stat">${D.meta.dataset}</div>
          <div class="dataset-meta">semnături ECDSA, fiecare cu un trace sintetic de ${D.meta.traceLength} eșantioane. Pentru atac: ${D.meta.test} ID-uri în test.</div>
        </div>
        <div class="dataset-signal-board">
          <div class="dataset-board-head"><span>TRACE FIELD / 256 SAMPLES</span><span>96 · 24 · 40</span></div>
          <div class="dataset-bands">
            <section class="dataset-band dataset-band--train">
              <div class="dataset-band-label"><strong>TRAIN</strong><span>96 traces</span></div>
              <svg viewBox="0 0 760 260" role="img" aria-label="Câmp de trace-uri pentru setul de antrenare">${train.join('')}</svg>
            </section>
            <section class="dataset-band dataset-band--validation">
              <div class="dataset-band-label"><strong>VALIDATION</strong><span>24 traces</span></div>
              <svg viewBox="0 0 760 110" role="img" aria-label="Câmp de trace-uri pentru validare">${validation.join('')}</svg>
            </section>
            <section class="dataset-band dataset-band--attack">
              <div class="dataset-band-label"><strong>ATTACK</strong><span>40 traces</span></div>
              <svg viewBox="0 0 760 150" role="img" aria-label="Câmp de trace-uri pentru atac">${attack.join('')}</svg>
            </section>
          </div>
          <div class="dataset-axis"><span>0</span><span>64</span><span>128</span><span>192</span><span>256</span></div>
        </div>
      </div>
      <div class="scene-note"><span>TARGET = PRIMELE 12 POZIȚII / MSB PREFIX</span><strong>Același format de trace trece prin splitul train → validation → attack; câmpul vizual arată distribuția, nu o nouă măsurătoare.</strong></div>
    `);
  }

  function renderLeakage() {
    const trace = D.meta.trace;
    const windowSize = 12;
    const frameStarts = [0, 24, 48, 72, 96, 120, 144, 168, 192, 216, 240];
    const overviewX = frameStarts.map(i => (24 + (i / (trace.length - 1)) * 932).toFixed(1));
    const zoomFrames = frameStarts.map(start => {
      const vals = trace.slice(start, start + windowSize);
      while (vals.length < windowSize) vals.push(trace[trace.length - 1]);
      return tracePath(vals, 620, 170, 14, 16);
    });
    const rectXValues = overviewX.join(';');
    const zoomPathValues = zoomFrames.join(';');
    const overview = svgEl('0 0 980 260', 'Trace sintetic de 256 samples cu fereastră de zoom dinamică', `
      <g>${[40,90,140,190,240].map(y=>`<path class="scope-grid-line" d="M24 ${y}H956"/>`).join('')}${[120,240,360,480,600,720,840].map(x=>`<path class="scope-grid-line" d="M${x} 22V238"/>`).join('')}</g>
      <path class="scope-trace-ghost" d="${tracePath(trace,980,260,24,22)}"/>
      <path class="scope-trace" d="${tracePath(trace,980,260,24,22)}"/>
      <rect class="scope-window" x="${overviewX[0]}" y="20" width="42" height="220" rx="1">
        <animate attributeName="x" values="${rectXValues}" dur="7.2s" repeatCount="indefinite" calcMode="linear"/>
      </rect>
      <rect class="scope-window-handle" x="${Number(overviewX[0])-3.5}" y="92" width="7" height="7">
        <animate attributeName="x" values="${overviewX.map(x => (Number(x)-3.5).toFixed(1)).join(';')}" dur="7.2s" repeatCount="indefinite" calcMode="linear"/>
      </rect>
      <rect class="scope-window-handle" x="${Number(overviewX[0])+38.5}" y="92" width="7" height="7">
        <animate attributeName="x" values="${overviewX.map(x => (Number(x)+38.5).toFixed(1)).join(';')}" dur="7.2s" repeatCount="indefinite" calcMode="linear"/>
      </rect>
      <text x="28" y="250" fill="#5c5851" font-size="8" font-family="IBM Plex Mono">0</text>
      <text x="475" y="250" fill="#5c5851" font-size="8" font-family="IBM Plex Mono">128</text>
      <text x="932" y="250" fill="#5c5851" font-size="8" font-family="IBM Plex Mono">256</text>
    `);
    const zoom = svgEl('0 0 620 170', 'Fereastra locală de 12 samples, sincronizată cu selecția de sus', `
      <g>${[30,70,110,150].map(y=>`<path class="scope-grid-line" d="M12 ${y}H608"/>`).join('')}</g>
      <path class="scope-zoom-wave" d="${zoomFrames[0]}">
        <animate attributeName="d" values="${zoomPathValues}" dur="7.2s" repeatCount="indefinite" calcMode="linear"/>
      </path>
      <line x1="14" y1="151" x2="606" y2="151" stroke="rgba(216,208,194,.10)"/>
    `);
    return sceneShell('scene--leakage', `
      <div class="leakage-layout">
        <div class="leakage-side">
          <div class="scene-header"><div><div class="scene-code">04 · LEAKAGE</div><div class="scene-title">Nonce-ul devine <em>semnal</em>.</div></div><div class="scene-source">REPO · ecdsa_leakage_model4.py<br>PAPER · §3.4</div></div>
          <div class="leakage-formula">HW(x) = popcount(x)</div>
          <div class="leakage-formula">HD(a,b) = popcount(a ⊕ b)</div>
          <div class="leakage-params">MODEL = ${D.meta.leakageModel}<br>σ = ${D.meta.sigma}<br>TRACE = ${D.meta.traceLength} samples<br>SOURCE = SYNTHETIC</div>
        </div>
        <div class="scope-board">
          <div class="scope-top"><span>OSCILLOSCOPE / OVERVIEW</span><span>256 samples · moving 12-sample window</span></div>
          <div class="scope-overview">${overview}</div>
          <div class="scope-zoom"><div class="scope-zoom-head"><span>LOCAL WINDOW / LIVE ZOOM</span><span>12 positions</span></div>${zoom}<div class="scope-zoom-note">Fereastra de sus și graficul de jos sunt aceeași porțiune a trace-ului; selecția se deplasează sincron.</div></div>
        </div>
      </div>
      <div class="scene-note"><span>THE SIGNAL IS TEMPORAL</span><strong>Modelul de bază folosește HW cu zgomot gaussian; nu este o măsurătoare fizică ESP32.</strong></div>
    `);
  }

  function cnnSvg() {
    const W=1120, H=430;
    const inputX=42, inputY=118, cellW=20, cellH=18, gap=5;
    const inputEnd = inputX + 12*cellW + 11*gap;
    const convX=360, convY=104, convW=120, convH=112;
    const featureX=545, featureY=62, featureW=210, mapH=15, mapGap=13;
    const reluX=815, reluY=104, reluW=108, reluH=112;
    const outX=1010, outY=122;
    const cells = (row) => Array.from({length:12},(_,i)=>{
    const x=inputX+i*(cellW+gap);
    const y=inputY+row*(cellH+18);
    return `<rect class="cnn-input-cell ${i%3===0?'is-active':''}" x="${x}" y="${y+15}" width="${cellW}" height="${cellH}" rx="1"/><text class="cnn-cell-label" x="${x+cellW/2}" y="${y+27}" text-anchor="middle">${i+1}</text>`;
    }).join('');
    const maps = Array.from({length:8},(_,i)=>{
      const y=featureY+i*(mapH+mapGap);
      return `<rect class="cnn-map" x="${featureX}" y="${y}" width="${featureW}" height="${mapH}" rx="2"/><text class="cnn-map-label" x="${featureX+10}" y="${y+10.5}">F${i+1}</text><line class="cnn-map-signal ${i===2||i===5?'is-live':''}" x1="${featureX+56}" y1="${y+7.5}" x2="${featureX+192}" y2="${y+7.5}"/>`;
    }).join('');
    const kernelStart = inputX-4;
    const kernelWidth = cellW*3 + gap*2 + 8;
    const kernelSteps = [kernelStart, kernelStart+75, kernelStart+150, kernelStart+225].join(';');
    return svgEl(`0 0 ${W} ${H}`, 'CNN 1D clarificat: două canale intră într-un kernel temporal, produc opt feature maps, apoi ReLU și două clase', `
      <g>
        <text class="cnn-label" x="42" y="60">INPUT / 2 CHANNELS</text>
        <text class="cnn-sub" x="42" y="84">raw</text>
        ${cells(0)}
        <text class="cnn-sub" x="42" y="100">difference</text>
        ${cells(1)}
        <rect class="cnn-kernel-clean" x="${kernelStart}" y="${inputY+8}" width="${kernelWidth}" height="${cellH*2+18+14}" rx="2"><animate attributeName="x" values="${kernelSteps}" dur="3.6s" repeatCount="indefinite"/></rect>
        <text class="cnn-kernel-tag" x="42" y="214">k = 3 / temporal window</text>
        <text class="cnn-kernel-tag" x="${inputEnd-38}" y="214" text-anchor="end">L = 12</text>
      </g>
      <g class="cnn-flow-arrow">
      <path d="M325 160H355"/>
      <path d="M345 152L355 160L345 168"/>
      </g>
      <g>
        <rect class="cnn-block" x="${convX}" y="${convY}" width="${convW}" height="${convH}" rx="2"/>
        <text class="cnn-block-title" x="${convX+18}" y="${convY+30}">CONV1D</text>
        <text class="cnn-block-main" x="${convX+18}" y="${convY+58}">8 maps</text>
        <text class="cnn-block-sub" x="${convX+18}" y="${convY+80}">kernel = 3</text>
        <text class="cnn-block-sub" x="${convX+18}" y="${convY+96}">stride = 1</text>
      </g>
      <g class="cnn-flow-arrow"><path d="M495 160H525"/><path d="M515 152L525 160L515 168"/></g>
      <g>
        <text class="cnn-label" x="${featureX}" y="46">FEATURE MAPS / 8 CHANNELS</text>
        ${maps}
      </g>
      <g class="cnn-flow-arrow"><path d="M770 160H800"/><path d="M790 152L800 160L790 168"/></g>
      <g>
        <rect class="cnn-block" x="${reluX}" y="${reluY}" width="${reluW}" height="${reluH}" rx="2"/>
        <text class="cnn-block-title" x="${reluX+17}" y="${reluY+31}">ReLU</text>
        <text class="cnn-block-main" x="${reluX+17}" y="${reluY+61}">max(0,x)</text>
        <text class="cnn-block-sub" x="${reluX+17}" y="${reluY+86}">non-linearity</text>
      </g>
      <g class="cnn-flow-arrow"><path d="M938 160H978"/><path d="M966 152L978 160L966 168"/></g>
      <g>
        <text class="cnn-label" x="955" y="70">1×1 → 2 CLASSES</text>
        <circle class="cnn-output-clean" cx="${outX}" cy="${outY}" r="27"/>
        <circle class="cnn-output-clean is-hot" cx="${outX}" cy="${outY+76}" r="27"/>
        <text class="cnn-output-label" x="${outX}" y="${outY+4}" text-anchor="middle">0</text>
        <text class="cnn-output-label" x="${outX}" y="${outY+80}" text-anchor="middle">1</text>
        <text class="cnn-sub" x="${outX+38}" y="${outY+4}">bit = 0</text>
        <text class="cnn-sub" x="${outX+38}" y="${outY+80}">bit = 1</text>
      </g>
      <g>
        <text class="cnn-axis-note" x="42" y="286">ONLY TIME AXIS</text>
        <path class="cnn-axis-line" d="M42 298H1070"/>
        <path class="cnn-axis-arrow" d="M1055 292L1070 298L1055 304"/>
        <text class="cnn-axis-note" x="42" y="324">12 positions in the base run → one local receptive field at a time</text>
      </g>
    `);
  }

  function renderCNN() {
    return sceneShell('scene--cnn', `
      <div class="cnn-layout">
        <div class="cnn-head">
          <div><div class="scene-code">05 · CNN 1D</div><div class="cnn-title">Nu imagine. Nu 2D.<br><em>O secvență care alunecă.</em></div></div>
          <div class="cnn-badge">REPO CONFIG<strong>Conv1d · 2 → 8 → 2</strong><div style="margin-top:5px">kernel 3 · ReLU · 1×1</div></div>
        </div>
        <div class="cnn-figure">
          ${cnnSvg()}
          <div class="cnn-1d-strip"><span>TIME / POSITION →</span><span>L = 12 IN BASE RUN</span></div>
        </div>
        <div class="cnn-kernel-caption">Fereastra citește trei poziții consecutive din cele două canale <strong>raw + difference</strong>, apoi transformarea rămâne 1D până la cele două clase de ieșire.</div>
      </div>
      <div class="scene-note"><span>MODEL = NONCEBITCNN</span><strong>Diagrama separă clar inputul, operația Conv1d, feature maps, ReLU și clasificarea binară.</strong></div>
    `);
  }

  function renderInference() {
    const bits = D.meta.testExample.prefix.split('').map(Number);
    const row = (label, values, extra='') => `<div class="bit-row"><div class="bit-row-label">${label}<br><small>${extra}</small></div><div class="bit-cells">${values.map((b,i)=>`<div class="bit-cell ${b?'is-one':''}" style="--delay:${i*90}ms">${b}</div>`).join('')}</div></div>`;
    return sceneShell('scene--inference', `
      <div class="inference-layout">
        <div class="inference-copy">
          <div class="scene-header"><div><div class="scene-code">06 · INFERENȚĂ</div><div class="scene-title">Din urmă apar <em>12 decizii</em>.</div></div><div class="scene-source">REPO · cnn_nonce_analysis5.py<br>TEST = ${D.meta.test} samples</div></div>
          <div class="inference-number">${D.meta.leakedBits}<span>b</span></div>
          <p>Rețeaua nu produce cheia privată. Produce un prefix estimat al nonce-ului. Acesta este singurul lucru pe care îl transmitem mai departe spre HNP.</p>
          <div class="inference-foot"><span>VAL BIT <strong>${D.meta.metrics.valBit.toFixed(2)}%</strong></span><span>TEST PREFIX <strong>${D.meta.metrics.testPrefix.toFixed(0)}%</strong></span></div>
        </div>
        <div class="bit-bridge">
          ${row('GROUND TRUTH', bits, `test sample ${D.meta.testExample.sampleId} / first 12`)}
          <div class="bit-separator"></div>
          ${row('MODEL OUTPUT', bits, 'CNN prediction / exact prefix in archive snapshot')}
          <div class="inference-foot"><span>MODEL → PREFIX</span><span><strong>40 / 40</strong> test prefixes exact în snapshot</span></div>
        </div>
      </div>
      <div class="scene-note"><span>BIT CLASSIFICATION ≠ KEY RECOVERY</span><strong>Prefixele devin apoi valori āᵢ în instanța HNP.</strong></div>
    `);
  }

  function renderHnp() {
    const rows = ['tᵢ d + uᵢ − aᵢ = δᵢ (mod n)','aᵢ = k̄ᵢ 2^(256−ℓ)','0 ≤ δᵢ < B','B = 2^(256−ℓ)'].map((x,i)=>`<div class="constraint-row"><span>0${i+1}</span><div>${x}</div><small>${i===3?'uncertainty bound':'derived'}</small></div>`).join('');
    return sceneShell('scene--hnp', `
      <div class="hnp-layout">
        <div class="hnp-copy">
          <div class="scene-header"><div><div class="scene-code">07 · HNP</div><div class="hnp-title">Biții devin<br><em>constrângeri.</em></div></div><div class="scene-source">REPO · hnp_attack1.py<br>PAPER · §3.2</div></div>
          <div class="hnp-steps"><div class="hnp-step"><span>01</span><strong>prefixul k̄ᵢ</strong> intră în ecuație</div><div class="hnp-step"><span>02</span><strong>tᵢ, uᵢ, aᵢ</strong> leagă fiecare semnătură de d</div><div class="hnp-step"><span>03</span><strong>40 relații</strong> împing aceeași necunoscută globală</div></div>
        </div>
        <div class="hnp-system">
          <div class="equation-hero">CNN → <strong>k̄ᵢ</strong> → <strong>aᵢ</strong> → HNP</div>
          <div class="constraint-list">${rows}</div>
          <div class="hnp-bridge-arrow">NEXT: <strong>m = 40 → lattice dimension = m + 2 = 42</strong></div>
        </div>
      </div>
      <div class="scene-note"><span>HIDDEN NUMBER PROBLEM</span><strong>Nu „scoatem” cheia din CNN; schimbăm forma problemei astfel încât lattice-ul să poată căuta un candidat.</strong></div>
    `);
  }

  function latticeSvg() {
    const rawVectors = [
      [78,290,250,168],[78,290,318,246],[78,290,286,120],[78,290,372,208],
      [78,290,230,92],[78,290,402,270],[78,290,344,142],[78,290,190,222]
    ];
    const reducedVectors = [
      [625,292,744,236],[625,292,780,272],[625,292,716,184],[625,292,812,304]
    ];
    const rawDots = Array.from({length:26},(_,i)=>{
      const x=92+((i*47)%390), y=76+((i*71)%235);
      return `<circle class="lattice-dot-soft" cx="${x}" cy="${y}" r="${i%5===0?2.8:1.7}"/>`;
    }).join('');
    const reducedDots = Array.from({length:16},(_,i)=>{
      const x=650+((i*31)%180), y=166+((i*43)%150);
      return `<circle class="lattice-dot-soft" cx="${x}" cy="${y}" r="1.8"/>`;
    }).join('');
    return svgEl('0 0 900 420','LLL: de la o bază aglomerată la o bază redusă, cu un candidat verificabil',`
      <g class="lattice-grid">
        <path d="M55 320H430M55 260H430M55 200H430M55 140H430M55 80H430"/>
        <path d="M110 55V340M170 55V340M230 55V340M290 55V340M350 55V340M410 55V340"/>
        <path d="M570 320H855M570 270H855M570 220H855M570 170H855"/>
        <path d="M620 145V340M680 145V340M740 145V340M800 145V340"/>
      </g>
      <g>${rawDots}</g>
      <g>${rawVectors.map(v=>`<line class="lattice-vector lattice-vector--raw" x1="${v[0]}" y1="${v[1]}" x2="${v[2]}" y2="${v[3]}"/>`).join('')}</g>
      <g>${reducedDots}</g>
      <g>${reducedVectors.map((v,i)=>`<line class="lattice-vector lattice-vector--reduced ${i===0?'is-target':''}" x1="${v[0]}" y1="${v[1]}" x2="${v[2]}" y2="${v[3]}"/>`).join('')}</g>
      <g class="lattice-origin">
        <circle cx="78" cy="290" r="4"/><circle cx="625" cy="292" r="4"/>
      </g>
      <g class="lattice-target">
        <circle cx="744" cy="236" r="7"/>
        <circle cx="744" cy="236" r="12" class="lattice-target-ring"/>
      </g>
      <g class="lattice-transfer">
        <path d="M442 210H515"/><path d="M500 201L515 210L500 219"/>
        <text x="456" y="191">LLL</text>
      </g>
      <text class="lattice-label-svg" x="58" y="358">RAW BASIS</text>
      <text class="lattice-label-svg" x="58" y="372">m + 2 = 42D</text>
      <text class="lattice-label-svg" x="570" y="358">REDUCED BASIS</text>
      <text class="lattice-label-svg" x="570" y="372">SHORTER / STRUCTURED</text>
      <text class="lattice-label-svg lattice-candidate-label" x="760" y="228">CANDIDATE</text>
    `);
  }

  function renderLll() {
    return sceneShell('scene--lll', `
      <div class="lll-layout">
        <div class="lll-copy">
          <div class="scene-header"><div><div class="scene-code">08 · LLL</div><div class="lll-title">Nu privim mai bine.<br><em>Reorientăm spațiul.</em></div></div><div class="scene-source">REPO · Lattice_key6.py<br>PAPER · §3.3 / E4</div></div>
          <div class="lll-note">Pentru m = 40, embedding-ul are dimensiunea m + 2 = 42. LLL reduce baza; candidatul este acceptat doar după verificarea independentă a relațiilor HNP.</div>
        </div>
        <div class="lattice-scene">${latticeSvg()}<div class="lll-status"><span>BEFORE <strong>many directions</strong></span><span>AFTER <strong>shorter / structured vectors</strong></span></div></div>
      </div>
      <div class="scene-note"><span>ARCHIVE SNAPSHOT = PASS_DEVELOPMENT / SYMPY</span><strong>Lucrarea descrie calea de referință cu fpylll; site-ul nu confundă cele două contexte.</strong></div>
    `);
  }

  function renderValidation() {
    const rows = D.meta.paperSweep.filter(r=>r.ell===12).map(r=>`<tr class="${r.sigma===.15?'highlight':''}"><td>ℓ=${r.ell}, σ=${r.sigma.toFixed(2)}</td><td><strong>${r.bit.toFixed(2)}%</strong></td><td>${r.prefix.toFixed(2)}%</td><td>${r.oracle}</td><td>${r.cnn}</td></tr>`).join('');
    return sceneShell('scene--validation', `
      <div class="validation-layout">
        <div class="validation-lead">
          <div class="scene-header"><div><div class="scene-code">09 · VALIDARE</div><div class="validation-title">Rezultatul contează.<br><em>Limita lui la fel.</em></div></div><div class="scene-source">PAPER · §6–§9<br>TABLE 2</div></div>
          <div class="validation-summary">În snapshot-ul din arhivă, CNN-ul arată ${D.meta.metrics.testBit.toFixed(0)}% test bit accuracy și ${D.meta.metrics.testPrefix.toFixed(0)}% test prefix accuracy pe setul de 40. HNP-ul este etichetat development-level în artefacte.</div>
          <div class="validation-state"><i></i><span>SOFTWARE CHAIN = TRACE → CNN → HNP → LLL</span></div>
          <div class="validation-state"><i></i><span>HARDWARE = ESP32 / NEXT EXPERIMENT</span></div>
        </div>
        <div class="results-table-wrap">
          <div class="results-head"><strong>Paper sweep · ℓ = 12 · m = 40</strong><span>three independent seeds / configuration</span></div>
          <table class="results-table"><thead><tr><th>setting</th><th>bit acc.</th><th>prefix acc.</th><th>oracle</th><th>CNN→HNP</th></tr></thead><tbody>${rows}</tbody></table>
          <div class="boundary-note"><strong>Ce arată tabelul:</strong> pe măsură ce σ crește, acuratețea prefixului scade; lucrarea separă astfel robustețea lattice-ului de erorile introduse de predicțiile CNN. Pentru hardware real, protocolul cere traces și metadata fizice verificabile.</div>
        </div>
      </div>
      <div class="scene-note"><span>CLOSE THE LOOP</span><strong>Rezultatele sunt legate de surse; nimic din această animație nu revendică o măsurătoare fizică absentă din arhivă.</strong></div>
    `);
  }

  const renderers = [renderIdea, renderEcdsa, renderDataset, renderLeakage, renderCNN, renderInference, renderHnp, renderLll, renderValidation];

  function buildChapters() {
    chapterTrack.innerHTML = stages.map((s,i)=>`<button class="chapter-chip ${i===0?'is-active':''}" data-scene="${i}" type="button"><span>${s.number}</span><strong>${esc(s.shortTitle)}</strong><small>${esc(s.factStatus)}</small></button>`).join('');
    chapterTrack.querySelectorAll('[data-scene]').forEach(btn => btn.addEventListener('click', () => gotoScene(Number(btn.dataset.scene), true)));
  }

  function updatePaperRail(scene) {
    const target = paperStageFor(scene.id);
    paperStages.forEach(el => el.classList.toggle('is-active', el.dataset.paperStage===target));
  }

  function updateContext(scene) {
    sceneContext.index.textContent = scene.number;
    sceneContext.kicker.textContent = scene.kicker.replace(/^\d+\s·\s*/, '');
    sceneContext.title.textContent = scene.infoTitle;
    sceneContext.copy.textContent = scene.infoCopy;
    sceneContext.dataset.textContent = String(D.meta.dataset);
    sceneContext.cnn.textContent = 'Conv1d';
    sceneContext.cnnSub.textContent = `${D.meta.cnn.inChannels} → ${D.meta.cnn.outChannels} → ${D.meta.cnn.classes}`;
    sceneContext.hnp.textContent = `ℓ = ${D.meta.leakedBits}`;
    sceneContext.status.textContent = scene.factStatus;
    const statusMap = { 'PASS_DEVELOPMENT':'archive · development', 'VALIDATE · THEN EXTEND':'archive · bounded claim' };
    sceneContext.statusSub.textContent = statusMap[scene.factStatus] || 'archive snapshot';
    chapterTrack.querySelectorAll('.chapter-chip').forEach((el,i)=>el.classList.toggle('is-active',i===sceneIndex));
    updatePaperRail(scene);
  }

  function renderErrorScene(error) {
    const message = error instanceof Error ? error.message : String(error);
    return sceneShell('scene--error', `
      <div class="error-scene">
        <div class="scene-code">RENDER RECOVERY</div>
        <h1>Scena nu a putut fi desenată.</h1>
        <p>Playerul a rămas funcțional, iar eroarea este izolată de scena curentă.</p>
        <code>${esc(message)}</code>
      </div>
    `);
  }

  function renderScene({ autoplay=true, preserveTime=false }={}) {
    if (!stages.length) {
      stage.innerHTML = renderErrorScene(new Error('Scene metadata is missing.'));
      playing = false;
      updatePlaybackUi();
      return;
    }
    const renderer = renderers[sceneIndex];
    let markup;
    try {
      markup = renderer();
    } catch (error) {
      console.error('RoSEF scene render failed', error);
      markup = renderErrorScene(error);
    }
    stage.innerHTML = markup;
    const current = stage.firstElementChild;
    if (current) current.classList.add('morph-in');
    durationMs = Number(stages[sceneIndex].duration || 0) * 1000 / speed;
    if (!preserveTime) elapsedMs = 0;
    elapsedMs = clamp(elapsedMs, 0, durationMs);
    pausedAt = elapsedMs;
    startedAt = performance.now() - elapsedMs;
    playing = autoplay;
    updateContext(stages[sceneIndex]);
    updatePlaybackUi();
  }

  function updatePlaybackUi() {
    playIcon.textContent = playing ? 'Ⅱ' : '▶';
    playBtn.setAttribute('aria-label', playing ? 'Pauză' : 'Continuă');
    playBtn.setAttribute('aria-pressed', String(playing));
    loopBtn.classList.toggle('is-active', loop);
    loopBtn.setAttribute('aria-pressed', String(loop));
  }

  function currentElapsed() { return playing ? (performance.now() - startedAt) : pausedAt; }

  function tick(now) {
    const deltaElapsed = currentElapsed();
    if (playing && !scrubbing) {
      if (deltaElapsed >= durationMs) {
        nextScene();
        requestAnimationFrame(tick);
        return;
      }
      elapsedMs = deltaElapsed;
    }
    const ratio = durationMs > 0 ? clamp(elapsedMs / durationMs, 0, 1) : 0;
    progress.style.width = `${ratio * 100}%`;
    progressThumb.style.left = `${ratio * 100}%`;
    progressTrack.setAttribute('aria-valuenow', String(Math.round(ratio * 100)));
    stageClock.textContent = `${fmt((elapsedMs / 1000) * speed)} / ${fmt(stages[sceneIndex].duration)}`;
    const offset = stages.slice(0, sceneIndex).reduce((sum,s)=>sum+s.duration,0) + (elapsedMs/1000)*speed;
    reelTime.textContent = `${fmt(offset)} / ${fmt(reelDuration)}`;
    requestAnimationFrame(tick);
  }

  function gotoScene(index, autoplay=true) {
    sceneIndex = clamp(index, 0, stages.length-1);
    renderScene({autoplay});
  }

  function nextScene() {
    if (sceneIndex < stages.length - 1) gotoScene(sceneIndex + 1, true);
    else if (loop) gotoScene(0, true);
    else { elapsedMs = durationMs; pausedAt = elapsedMs; playing = false; updatePlaybackUi(); }
  }

  function prevScene() {
    if (elapsedMs > 650) { elapsedMs = 0; pausedAt = 0; startedAt = performance.now(); updatePlaybackUi(); return; }
    gotoScene(sceneIndex > 0 ? sceneIndex-1 : (loop ? stages.length-1 : 0), true);
  }

  function togglePlay() {
    if (playing) { pausedAt = currentElapsed(); elapsedMs = pausedAt; playing = false; }
    else { startedAt = performance.now() - elapsedMs; playing = true; }
    updatePlaybackUi();
  }

  function scrubFromClientX(clientX) {
    const rect = progressTrack.getBoundingClientRect();
    const ratio = clamp((clientX - rect.left) / rect.width, 0, 1);
    elapsedMs = ratio * durationMs;
    pausedAt = elapsedMs;
    startedAt = performance.now() - elapsedMs;
  }

  function setSpeed(nextSpeed) {
    if (!stages.length) return;
    const oldPosition = durationMs ? clamp(currentElapsed() / durationMs, 0, 1) : 0;
    speed = Number(nextSpeed);
    durationMs = Number(stages[sceneIndex].duration || 0) * 1000 / speed;
    elapsedMs = oldPosition * durationMs;
    pausedAt = elapsedMs;
    startedAt = performance.now() - elapsedMs;
  }

  function openSources() {
    const items = D.sourceMap.map(([label,path,extra,ref])=>`<div class="source-item"><div><strong>${esc(label)}</strong><code>${esc(path)}${extra ? `\n${esc(extra)}`:''}\n${esc(ref)}</code></div><a href="${esc(path)}" target="_blank" rel="noreferrer">OPEN ↗</a></div>`).join('');
    dialogBody.innerHTML = `<p class="source-warning"><strong>Reconciliare importantă:</strong> PDF-ul descrie CNN-ul ca one-dimensional. Implementarea actuală din repository folosește explicit <code>torch.nn.Conv1d</code> și config-ul <code>2 → 8 → 2, kernel 3</code>. PDF-ul păstrează în secțiunea 4 un listing cu default-uri mai vechi (1 channel, 16 hidden, kernel 9); animația urmează codul verificat din arhivă.</p>${items}<p class="source-warning"><strong>HNP:</strong> artefactele furnizate în arhivă sunt <code>PASS_DEVELOPMENT</code> cu backend SymPy; lucrarea descrie calea de referință cu fpylll. Nu le-am fuzionat într-un singur status.</p>`;
    if (!dialog.open) dialog.showModal();
  }

  playBtn.addEventListener('click', togglePlay);
  nextBtn.addEventListener('click', nextScene);
  prevBtn.addEventListener('click', prevScene);
  restartBtn.addEventListener('click', () => renderScene({autoplay:true}));
  loopBtn.addEventListener('click', () => { loop=!loop; updatePlaybackUi(); });
  speedSelect.addEventListener('change', e => setSpeed(e.target.value));
  progressTrack.addEventListener('pointerdown', e => { scrubbing=true; progressTrack.setPointerCapture(e.pointerId); scrubFromClientX(e.clientX); });
  progressTrack.addEventListener('pointermove', e => { if(scrubbing) scrubFromClientX(e.clientX); });
  progressTrack.addEventListener('pointerup', () => { scrubbing=false; });
  progressTrack.addEventListener('pointercancel', () => { scrubbing=false; });
  progressTrack.addEventListener('lostpointercapture', () => { scrubbing=false; });
  progressTrack.addEventListener('keydown', e => {
    if (e.key === 'ArrowRight' || e.key === 'ArrowLeft') {
      e.preventDefault();
      e.stopPropagation();
      const step = e.key === 'ArrowRight' ? 1000 : -1000;
      elapsedMs = clamp(elapsedMs + step, 0, durationMs);
      pausedAt = elapsedMs;
      startedAt = performance.now() - elapsedMs;
    }
  });
  fullscreenBtn.addEventListener('click', () => {
    const el = document.querySelector('.player');
    if (!document.fullscreenElement) el.requestFullscreen?.().catch?.(() => {}); else document.exitFullscreen?.().catch?.(() => {});
  });
  sourcesBtn.addEventListener('click', openSources);
  sourceNoteBtn.addEventListener('click', openSources);
  paperPageBtn.addEventListener('click', () => window.open('paper/Recovery_of_private_keys.pdf', '_blank', 'noopener'));
  dialogClose.addEventListener('click', () => dialog.close());
  dialog.addEventListener('click', e => { if(e.target === dialog) dialog.close(); });

  document.addEventListener('keydown', e => {
    if (dialog.open) return;
    if (e.target.matches('input, select, textarea, [contenteditable="true"], [role="slider"]')) return;
    if (e.code === 'Space') { e.preventDefault(); togglePlay(); }
    else if (e.key === 'ArrowRight') { e.preventDefault(); nextScene(); }
    else if (e.key === 'ArrowLeft') { e.preventDefault(); prevScene(); }
    else if (e.key === 'Home') { e.preventDefault(); gotoScene(0, true); }
    else if (e.key === 'End') { e.preventDefault(); gotoScene(stages.length-1, true); }
    else if (e.key.toLowerCase() === 'f') { e.preventDefault(); fullscreenBtn.click(); }
  });

  buildChapters();
  renderScene();
  requestAnimationFrame(tick);
})();
