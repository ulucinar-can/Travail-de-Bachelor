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
    li a { display: block; color: white; text-align: center; padding: 14px 16px; text-decoration: none; transition: color 0.3s; cursor: pointer; font-weight: bold; }
    li a:hover { background-color: #d4ac0d; color: black; }
    
    .titre1 { color: white; font-size: 200%; font-family: verdana; text-align: center; }
    .titre2 { color: white; font-size: 150%; font-family: verdana; text-align: center; color: #d4ac0d; }

    .button { border: none; color: white; padding: 12px 28px; text-align: center; text-decoration: none; display: inline-block; font-size: 30px; margin: 15px 4px; transition-duration: 0.4s; cursor: pointer; border-radius: 12px; font-weight: bold; }
    .start-btn { background-color: white; color: #04AA6D; border: 4px solid #04AA6D; }
    .start-btn:hover { background-color: #04AA6D; color: white; }
    .stop-btn { background-color: white; color: #F44336; border: 4px solid #F44336; }
    .stop-btn:hover { background-color: #F44336; color: white; }

    /* Tableau adaptatif pour les 4 Inducteurs */
    .inducteurs-colonnes { display: flex; justify-content: center; gap: 40px; margin: 20px auto; font-family: Verdana, sans-serif; color: white; flex-wrap: wrap; }
    .bloc-colonne { display: flex; flex-direction: column; align-items: center; }
    .entete { font-weight: bold; font-size: 18px; margin-bottom: 10px; text-align: center; color: #00e5ff; }
    .valeur { font-size: 16px; background-color: rgba(255, 255, 255, 0.1); border: 1px solid rgba(255,255,255,0.3); border-radius: 6px; padding: 6px 12px; margin: 4px 0; width: 70px; text-align: center; }

    /* Graphique */
    .graph-container { width: 100%; max-width: 800px; margin: auto; background-color: rgba(255, 255, 255, 0.05); border: 1px solid rgba(255, 255, 255, 0.2); border-radius: 10px; padding: 15px; box-sizing: border-box; }
    .legend { display: flex; justify-content: center; gap: 20px; margin-top: 15px; font-size: 14px; color: white; flex-wrap: wrap; }
    .legend-item { display: flex; align-items: center; gap: 8px; }
    .legend-color { width: 20px; height: 4px; border-radius: 2px; }

    /* Responsive (Téléphone) */
    @media (max-width: 600px) { 
      .inducteurs-colonnes { gap: 15px; } 
      .entete { font-size: 14px; } 
      .valeur { font-size: 14px; width: 50px; padding: 4px; } 
      .button { font-size: 24px; padding: 10px 20px; }
    }

    .page { display: none; }
    .active { display: block; }
    .container { max-width: 900px; margin: 20px auto; padding: 0 16px; text-align: center; }
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
      <h1 class="titre1">Sustentateur magnétique à 4 inducteurs</h1>
      <p style="font-size: 18px; line-height: 1.6;">Projet de contrôle et d'acquisition de données en temps réel via ESP32-S2.</p>
    </div>

    <div id="page-start" class="page">
      <h2 class="titre2">État de la maquette : <span id="etatBouton" style="color: white;">À l'arrêt</span></h2> 
      
      <div class="inducteurs-colonnes">
        <div class="bloc-colonne">
          <div class="entete">Data</div>
          <div class="valeur" style="border:none; font-weight:bold;">Pos 1</div>
          <div class="valeur" style="border:none; font-weight:bold;">Pos 2</div>
          <div class="valeur" style="border:none; font-weight:bold;">Pos 3</div>
          <div class="valeur" style="border:none; font-weight:bold;">Pos 4</div>
        </div>
        <div class="bloc-colonne">
          <div class="entete">Position [mm]</div>
          <div class="valeur" id="valInd1">0.000</div>
          <div class="valeur" id="valInd2">0.000</div>
          <div class="valeur" id="valInd3">0.000</div>
          <div class="valeur" id="valInd4">0.000</div>
        </div>
        <div class="bloc-colonne">
          <div class="entete">Courant [A]</div>
          <div class="valeur" id="curInd1">0.000</div>
          <div class="valeur" id="curInd2">0.000</div>
          <div class="valeur" id="curInd3">0.000</div>
          <div class="valeur" id="curInd4">0.000</div>
        </div>
      </div>

      <div class="graph-container">
        <canvas id="posGraph" width="800" height="350" style="width: 100%; height: auto; display: block;"></canvas>
        <div class="legend">
           <div class="legend-item"><div class="legend-color" style="background-color: #ff4d4d;"></div>Position 1</div>
           <div class="legend-item"><div class="legend-color" style="background-color: #00e5ff;"></div>Position 2</div>
           <div class="legend-item"><div class="legend-color" style="background-color: #ff9100;"></div>Courant 1</div>
           <div class="legend-item"><div class="legend-color" style="background-color: #b2ff59;"></div>Courant 2</div>
        </div>
      </div>

      <div style="text-align:center;">
        <input type="button" id="mixBut" value="Start" class="button start-btn" title="Lancer la sustentation"/>
      </div>
    </div>

    <div id="page-about" class="page">
      <h1 class="titre1">Contexte</h1>
      <p style="text-align: left; max-width: 700px; margin: auto; line-height: 1.6;">
        La page internet que vous utilisez actuellement fait partie d'un projet de bachelor et sert principalement comme moyen de démarrer le processus d'une maquette à distance et à observer des valeurs en continu.<br><br>
        <strong>Mise en service :</strong><br>
        1) Allumer l'alimentation 48V (limitation de courant à 3.5A).<br>
        2) Compiler la dernière version du code dans le DSP.<br>
        3) Attendre que le condensateur se charge, puis appuyer sur "Start" ici.
      </p>
      <br><br><small>Auteur : Thomas Freyche | Version 2.0 (8 Variables)</small>
    </div>
  </div>
  
<script type="text/javascript">
function afficherPage(id) {
  document.querySelectorAll('.page').forEach(p => p.classList.remove('active'));
  document.getElementById(id).classList.add('active');
}

window.onload = function() {
    var xmlHttp = new XMLHttpRequest();
    var processInterval = null;
    var mixBut = document.getElementById("mixBut");
    var etatSpan = document.getElementById("etatBouton");

    // Variables pour le graphique
    var histP1 = [], histP2 = [], histC1 = [], histC2 = [], timeData = [];
    var startTime = Date.now();
    var windowSize = 10.0; // Affiche les 10 dernières secondes

    mixBut.addEventListener("click", Start);

    // Fonction de traçage du graphique
    function renderGraph() {
        var canvas = document.getElementById("posGraph");
        if(!canvas) return;
        var ctx = canvas.getContext("2d");
        var w = canvas.width, h = canvas.height;
        ctx.clearRect(0, 0, w, h);

        var padL = 50, padB = 40, padT = 20, padR = 20;
        var plotW = w - padL - padR, plotH = h - padT - padB;
        var maxY = 10.0; // Échelle max du graphique (Ajuste si besoin)

        // Grille horizontale
        ctx.fillStyle = "white"; ctx.font = "12px Verdana"; ctx.textAlign = "right"; ctx.textBaseline = "middle";
        for(var i=0; i<=5; i++) {
            var val = i * (maxY / 5.0);
            var y = padT + plotH - (i/5.0)*plotH;
            ctx.strokeStyle = "rgba(255, 255, 255, 0.15)"; ctx.lineWidth = 1;
            ctx.beginPath(); ctx.moveTo(padL, y); ctx.lineTo(w-padR, y); ctx.stroke();
            ctx.fillText(val.toFixed(1), padL - 10, y);
        }

        // Grille verticale (Temps)
        var tStart = 0, tEnd = windowSize;
        if (timeData.length > 0) {
            var currentMaxTime = timeData[timeData.length - 1];
            if (currentMaxTime > windowSize) { tEnd = currentMaxTime; tStart = currentMaxTime - windowSize; }
        }
        var tSpan = Math.max(tEnd - tStart, 1);

        ctx.textAlign = "center"; ctx.textBaseline = "top";
        for(var j=0; j<=5; j++) {
            var x = padL + (j/5)*plotW;
            ctx.strokeStyle = "rgba(255, 255, 255, 0.15)";
            ctx.beginPath(); ctx.moveTo(x, padT); ctx.lineTo(x, h-padB); ctx.stroke();
            ctx.fillText((tStart + (j/5)*tSpan).toFixed(1) + "s", x, h - padB + 10);
        }

        // Zone de tracé
        ctx.save(); ctx.beginPath(); ctx.rect(padL, padT, plotW, plotH); ctx.clip(); 

        function traceCurve(data, color) {
            if (data.length === 0) return;
            ctx.beginPath(); ctx.strokeStyle = color; ctx.lineWidth = 2.5; ctx.lineJoin = "round";
            for(var k=0; k<data.length; k++) {
                var px = padL + ((timeData[k] - tStart) / tSpan) * plotW;
                var py = padT + plotH - (Math.min(Math.max(data[k], 0), maxY) / maxY) * plotH;
                if(k===0) ctx.moveTo(px, py); else ctx.lineTo(px, py);
            }
            ctx.stroke();
        }

        traceCurve(histP1, "#ff4d4d"); // Rouge : Pos 1
        traceCurve(histP2, "#00e5ff"); // Bleu : Pos 2
        traceCurve(histC1, "#ff9100"); // Orange : Cur 1
        traceCurve(histC2, "#b2ff59"); // Vert : Cur 2
        ctx.restore(); 
    }

    renderGraph(); // Dessine la grille vide au lancement

    function updateEtatText(etat) {
      etatSpan.textContent = etat === "1" ? "En mouvement" : "À l'arrêt";
    }
    
    function Start(){
      mixBut.removeEventListener("click", Start);
      mixBut.addEventListener("click", Stop);
      mixBut.value = "Stop";
      mixBut.classList.remove("start-btn");
      mixBut.classList.add("stop-btn");
      updateEtatText("1");

      // Reset graphique
      histP1 = []; histP2 = []; histC1 = []; histC2 = []; timeData = [];
      startTime = Date.now();

      // Requête Asynchrone (Non bloquante)
      fetch('/BUTTON_START', { method: 'PUT' }).catch(err => console.error(err));
      
      // Lance l'interrogation des données
      if (!processInterval) { processInterval = setInterval(process, 100); }
    }

    function Stop(){
      mixBut.removeEventListener("click", Stop);
      mixBut.addEventListener("click", Start);
      mixBut.value = "Start";
      mixBut.classList.remove("stop-btn");
      mixBut.classList.add("start-btn");
      updateEtatText("0");

      fetch('/BUTTON_STOP', { method: 'PUT' }).catch(err => console.error(err));
      
      if (processInterval) {
        clearInterval(processInterval);
        processInterval = null;
      }
    }

    function response() {
      if (xmlHttp.readyState != 4 || !xmlHttp.responseXML) return;
      var xmlResponse = xmlHttp.responseXML;
      
      // Fonction utilitaire pour extraire l'XML proprement
      var extractVal = function(tag) {
          var nodes = xmlResponse.getElementsByTagName(tag);
          if (nodes.length > 0 && nodes[0].firstChild) {
              return parseFloat(nodes[0].firstChild.nodeValue.replace(',', '.'));
          }
          return 0.0;
      };

      // Extraction des 8 valeurs (Pos 1..4 et Cur 1..4)
      var v_p1 = extractVal("valInd1");
      var v_p2 = extractVal("valInd2");
      var v_p3 = extractVal("valInd3");
      var v_p4 = extractVal("valInd4");
      
      var v_c1 = extractVal("curInd1");
      var v_c2 = extractVal("curInd2");
      var v_c3 = extractVal("curInd3");
      var v_c4 = extractVal("curInd4");

      // Mise à jour de l'affichage HTML
      document.getElementById("valInd1").innerHTML = v_p1.toFixed(3);
      document.getElementById("valInd2").innerHTML = v_p2.toFixed(3);
      document.getElementById("valInd3").innerHTML = v_p3.toFixed(3);
      document.getElementById("valInd4").innerHTML = v_p4.toFixed(3);
      
      document.getElementById("curInd1").innerHTML = v_c1.toFixed(3);
      document.getElementById("curInd2").innerHTML = v_c2.toFixed(3);
      document.getElementById("curInd3").innerHTML = v_c3.toFixed(3);
      document.getElementById("curInd4").innerHTML = v_c4.toFixed(3);

      // Mise à jour du graphique (On trace Pos1, Pos2, Cur1, Cur2)
      var currentGraphTime = (Date.now() - startTime) / 1000.0;
      timeData.push(currentGraphTime);
      histP1.push(v_p1);
      histP2.push(v_p2);
      histC1.push(v_c1);
      histC2.push(v_c2);

      // Maintient la "fenêtre glissante" de 10 secondes max
      while(timeData.length > 0 && (currentGraphTime - timeData[0] > windowSize)) {
          timeData.shift(); 
          histP1.shift(); 
          histP2.shift(); 
          histC1.shift(); 
          histC2.shift();
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