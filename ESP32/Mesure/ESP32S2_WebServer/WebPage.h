// --- Fichier WebPage.h ---
const char PAGE_MAIN[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="fr">
<head>
  <title>Interface utilisateur</title>
  <meta charset="UTF-8">  
  <meta name="description" content="Interface utilisateur pour démonstrateur">
  <meta name="author" content="Freyche Thomas">
  <meta name="viewport" content="width=device-width, initial-scale=1.0"> 

  <style>
    body { margin: 0; font-family: "Segoe UI", sans-serif; background: #2a5298; color: white; }
    ul { background-color: rgba(255, 255, 255, 0.1); list-style-type: none; margin: 0; padding: 0; overflow: hidden; display: flex; }
    li a { display: block; color: white; text-align: center; padding: 14px 16px; text-decoration: none; transition: color 0.3s; cursor: pointer; }
    li a:hover { background-color: #d4ac0d; color: black; }
    .titre1 { color: white; font-size: 200%; font-family: verdana; text-align: center; }
    .titre2 { color: white; font-size: 175%; font-family: verdana; text-align: center; }

    .button { border: none; color: white; padding: 12px 28px; text-align: center; text-decoration: none; display: inline-block; font-size: 30px; margin: 6px 4px; transition-duration: 0.4s; cursor: pointer; border-radius: 12px; }
    .start-btn { background-color: white; color: #04AA6D; border: 4px solid #04AA6D; }
    .start-btn:hover { background-color: #04AA6D; color: white; }
    .stop-btn { background-color: white; color: #F44336; border: 4px solid #F44336; }
    .stop-btn:hover { background-color: #F44336; color: white; }

    .inducteurs-colonnes { display: flex; justify-content: center; gap: 30px; margin: 20px auto; font-family: Verdana, sans-serif; color: white; flex-wrap: wrap; }
    .bloc-colonne { display: flex; flex-direction: column; align-items: center; }
    .entete { font-weight: bold; font-size: 18px; margin-bottom: 10px; text-align: center; }
    .valeur { font-size: 16px; background-color: rgba(255, 255, 255, 0.1); border: 1px solid white; border-radius: 6px; padding: 4px 8px; margin: 4px 0; width: 65px; text-align: center; }

    @media (max-width: 600px) { .inducteurs-colonnes { gap: 10px; } .entete { font-size: 14px; } .valeur { font-size: 14px; width: 55px; padding: 2px 4px; } }

    .page { display: none; }
    .active { display: block; }
    .container { max-width: 900px; margin: 40px auto; padding: 0 16px; text-align: center; }
  </style>
</head>

<body>
  <ul>
    <li><a onclick="afficherPage('page-home')">Home</a></li>
    <li><a onclick="afficherPage('page-start')">Mise en marche</a></li>
    <li><a onclick="afficherPage('page-about')">À propos</a></li>
  </ul>
  
  <div class="container">
    <div id="page-home" class="page active">
      <h1 class="titre1">Sustentateur magnétique par attraction</h1>
    </div>

    <div id="page-start" class="page">
      <h2 class="titre2">État du démonstrateur : <span id="etatBouton">À l'arrêt</span></h2> 
      <br>

      <div class="inducteurs-colonnes">
        <div class="bloc-colonne">
          <div class="entete">Data</div>
          <div class="valeur">Pos Consigne</div>
          <div class="valeur">Pos Mesure</div>
          <div class="valeur">Cur Consigne</div>
          <div class="valeur">Cur Mesure</div>
        </div>
        <div class="bloc-colonne">
          <div class="entete">Valeur</div>
          <div class="valeur" id="valInd1">0.000</div>
          <div class="valeur" id="valInd2">0.000</div>
          <div class="valeur" id="curInd1">0.000</div>
          <div class="valeur" id="curInd2">0.000</div>
        </div>
      </div>

      <br>
      <h3 style="color: white; margin-bottom: 15px; font-size: 18px;">Suivi temps réel (Inducteur 1)</h3>
      <div style="width: 100%; max-width: 800px; margin: auto; background-color: rgba(255, 255, 255, 0.05); border: 1px solid rgba(255, 255, 255, 0.2); border-radius: 10px; padding: 20px; box-sizing: border-box;">
        <canvas id="posGraph" width="800" height="400" style="width: 100%; height: auto; display: block;"></canvas>
        <div style="display: flex; justify-content: center; gap: 20px; margin-top: 15px; font-size: 14px; color: white; flex-wrap: wrap;">
           <div><span style="display: inline-block; width: 25px; height: 3px; background-color: #ff4d4d; vertical-align: middle; margin-right: 8px;"></span>Pos Consigne</div>
           <div><span style="display: inline-block; width: 25px; height: 3px; background-color: #00e5ff; vertical-align: middle; margin-right: 8px;"></span>Pos Mesure</div>
           <div><span style="display: inline-block; width: 25px; height: 3px; background-color: #2962ff; vertical-align: middle; margin-right: 8px;"></span>Courant Consigne</div>
           <div><span style="display: inline-block; width: 25px; height: 3px; background-color: #ff9100; vertical-align: middle; margin-right: 8px;"></span>Courant Mesure</div>
        </div>
      </div>
      <br>

      <div style="text-align:center;">
        <input type="button" id="mixBut" value="Start" class="button start-btn" title="Lancer la sustentation"/>
        <br><br>
        <input type="button" id="csvBut" value="Télécharger CSV (10s)" class="button" style="background-color: #2962ff; font-size: 20px; display: none;" onclick="downloadCSV()"/>
        <p id="csvStatus" style="color: #00e5ff; font-weight: bold;"></p>
      </div>
    </div>

    <div id="page-about" class="page">
      <h1>Contexte</h1>
      <p>Projet de bachelor pour la commande à distance et l'acquisition de données.</p>
    </div>
  </div>
  
<script type="text/javascript">
function afficherPage(id) {
  document.querySelectorAll('.page').forEach(p => p.style.display = 'none');
  document.getElementById(id).style.display = 'block';
}

var csvData = [];
var isRecording = false;
var recordStartTime = 0;

function downloadCSV() {
    var csvContent = csvData.map(e => e.join(";")).join("\n");
    var blob = new Blob([csvContent], { type: 'text/csv;charset=utf-8;' });
    var url = URL.createObjectURL(blob);
    var link = document.createElement("a");
    link.setAttribute("href", url);
    link.setAttribute("download", "mesure_10s_inducteur1.csv");
    document.body.appendChild(link);
    link.click();
    document.body.removeChild(link);
}

window.onload = function() {
    var xmlHttp = new XMLHttpRequest();
    var processInterval = null;
    var mixBut = document.getElementById("mixBut");
    var etatSpan = document.getElementById("etatBouton");

    mixBut.addEventListener("click", Start);

    function updateEtatText(etat) {
      etatSpan.textContent = etat === "1" ? "En mouvement" : "À l'arrêt";
    }

    var histP1 = [], histP2 = [], histP3 = [], histP4 = [], timeData = [];
    var startTime = Date.now();
    var windowSize = 10.0;

    function renderGraph() {
        var canvas = document.getElementById("posGraph");
        if(!canvas) return;
        var ctx = canvas.getContext("2d");
        var w = canvas.width, h = canvas.height;
        ctx.clearRect(0, 0, w, h);

        var padL = 70, padB = 60, padT = 20, padR = 20;
        var plotW = w - padL - padR, plotH = h - padT - padB;
        var maxY = 8.0; 

        ctx.fillStyle = "white"; ctx.font = "14px Verdana"; ctx.textAlign = "right"; ctx.textBaseline = "middle";

        for(var i=0; i<=4; i++) {
            var val = i * (maxY / 4.0);
            var y = padT + plotH - (i/4.0)*plotH;
            ctx.strokeStyle = "rgba(255, 255, 255, 0.15)"; ctx.lineWidth = 1;
            ctx.beginPath(); ctx.moveTo(padL, y); ctx.lineTo(w-padR, y); ctx.stroke();
            ctx.fillText(val.toFixed(1), padL - 10, y);
        }

        var tStart = 0, tEnd = windowSize;
        if (timeData.length > 0) {
            var currentMaxTime = timeData[timeData.length - 1];
            if (currentMaxTime > windowSize) { tEnd = currentMaxTime; tStart = currentMaxTime - windowSize; }
        }
        var tSpan = Math.max(tEnd - tStart, 1);

        ctx.textAlign = "center"; ctx.textBaseline = "top";
        for(var j=0; j<=5; j++) {
            var x = padL + (j/5)*plotW;
            ctx.strokeStyle = "rgba(255, 255, 255, 0.15)"; ctx.beginPath(); ctx.moveTo(x, padT); ctx.lineTo(x, h-padB); ctx.stroke();
            ctx.fillText((tStart + (j/5)*tSpan).toFixed(1) + "s", x, h - padB + 10);
        }

        ctx.save(); ctx.translate(padL - 50, padT + plotH/2); ctx.rotate(-Math.PI/2);
        ctx.textAlign = "center"; ctx.fillText("Valeur", 0, 0); ctx.restore();
        ctx.fillText("Temps [s]", padL + plotW/2, h - padB + 35);

        ctx.save(); ctx.beginPath(); ctx.rect(padL, padT, plotW, plotH); ctx.clip(); 

        function traceCurve(data, color) {
            if (data.length === 0) return;
            ctx.beginPath(); ctx.strokeStyle = color; ctx.lineWidth = 3; ctx.lineJoin = "round";
            for(var k=0; k<data.length; k++) {
                var px = padL + ((timeData[k] - tStart) / tSpan) * plotW;
                var py = padT + plotH - (Math.min(Math.max(data[k], 0), maxY) / maxY) * plotH;
                if(k===0) ctx.moveTo(px, py); else ctx.lineTo(px, py);
            }
            ctx.stroke();
        }

        traceCurve(histP1, "#ff4d4d"); traceCurve(histP2, "#00e5ff");
        traceCurve(histP3, "#2962ff"); traceCurve(histP4, "#ff9100");
        ctx.restore(); 
    }

    function resetGraph() {
        histP1 = []; histP2 = []; histP3 = []; histP4 = []; timeData = [];
        startTime = Date.now();
        renderGraph();
    }

    renderGraph();

    function Start(){
      mixBut.removeEventListener("click", Start);
      mixBut.addEventListener("click", Stop);
      mixBut.value = "Stop";
      mixBut.classList.remove("start-btn");
      mixBut.classList.add("stop-btn");
      updateEtatText("1");

      resetGraph(); 
      
      csvData = [["Temps [s]", "Pos_Consigne [mm]", "Pos_Mesure [mm]", "Cur_Consigne [A]", "Cur_Mesure [A]", "Force_Consigne", "Xr1", "Tension_Consigne [V]", "Integrale_PI", "State", "Vitesse [mm/s]"]];
      isRecording = true;
      recordStartTime = Date.now();
      
      document.getElementById("csvBut").style.display = "none";
      document.getElementById("csvStatus").innerText = "Acquisition en cours (10s)...";

      fetch('/BUTTON_START', { method: 'PUT' })
        .then(() => {
            if (!processInterval) { processInterval = setInterval(process, 50); }
        }).catch(err => console.error(err));
    }

    function Stop(){
      mixBut.removeEventListener("click", Stop);
      mixBut.addEventListener("click", Start);
      mixBut.value = "Start";
      mixBut.classList.remove("stop-btn");
      mixBut.classList.add("start-btn");
      updateEtatText("0");

      isRecording = false;

      fetch('/BUTTON_STOP', { method: 'PUT' })
        .then(() => {
            if (processInterval) { clearInterval(processInterval); processInterval = null; }
        });
    }

    function response() {
      if (xmlHttp.readyState != 4) return;
      var xmlResponse = xmlHttp.responseXML;
      if(!xmlResponse) return;

      var ext = function(tag) {
          var nodes = xmlResponse.getElementsByTagName(tag);
          if (nodes.length > 0 && nodes[0].firstChild) return parseFloat(nodes[0].firstChild.nodeValue.replace(',', '.'));
          return 0.0;
      };

      var v_posC1 = ext("posC1"), v_posM1 = ext("posM1");
      var v_curC1 = ext("curC1"), v_curM1 = ext("curM1");
      var v_fc1f = ext("fc1f"), v_xr1 = ext("xr1"), v_uc1 = ext("uc1"), v_intI1 = ext("intI1");
      var v_state = ext("state"), v_v1 = ext("v1");

      document.getElementById("valInd1").innerHTML = v_posC1.toFixed(3);
      document.getElementById("valInd2").innerHTML = v_posM1.toFixed(3);
      document.getElementById("curInd1").innerHTML = v_curC1.toFixed(3);
      document.getElementById("curInd2").innerHTML = v_curM1.toFixed(3);

      var currentTime = (Date.now() - recordStartTime) / 1000.0;

      if (isRecording) {
          csvData.push([
              currentTime.toFixed(3).replace('.', ','), v_posC1.toFixed(4).replace('.', ','),
              v_posM1.toFixed(4).replace('.', ','), v_curC1.toFixed(4).replace('.', ','),
              v_curM1.toFixed(4).replace('.', ','), v_fc1f.toFixed(4).replace('.', ','),
              v_xr1.toFixed(4).replace('.', ','), v_uc1.toFixed(4).replace('.', ','),
              v_intI1.toFixed(4).replace('.', ','), v_state, v_v1.toFixed(4).replace('.', ',')
          ]);

          if (currentTime >= 10.0) {
              isRecording = false;
              Stop();
              document.getElementById("csvStatus").innerText = "Acquisition de 10s terminée ! Fichier prêt.";
              document.getElementById("csvBut").style.display = "inline-block";
          }
      }

      var currentGraphTime = (Date.now() - startTime) / 1000.0;
      timeData.push(currentGraphTime);
      histP1.push(v_posC1); histP2.push(v_posM1); histP3.push(v_curC1); histP4.push(v_curM1);

      while(timeData.length > 0 && (currentGraphTime - timeData[0] > windowSize)) {
          timeData.shift(); histP1.shift(); histP2.shift(); histP3.shift(); histP4.shift();
      }
      renderGraph();
    }

    function process() {
     if(xmlHttp.readyState==0 || xmlHttp.readyState==4) {
        xmlHttp.open("PUT", "xml", true);
        xmlHttp.onreadystatechange = response;
        xmlHttp.send(null);
      }       
    }
};
</script>
</body>
</html>
)=====";