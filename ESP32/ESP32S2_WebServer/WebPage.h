const char PAGE_MAIN[] PROGMEM = R"=====(

<!DOCTYPE html>
<html lang="fr">
<head>
  <title>Interface utilisateur</title>
	<meta charset="UTF-8">	
	<meta name="description" content="Interface utilisateur pour démonstrateur">
	<meta name="keywords" content="HTML">
	<meta name="author" content="Freyche Thomas">
	<meta name="viewport" content="width=device-width, initial-scale=1.0"> <!--Make the web page look good on anydevice-->

	<style>
    
	  body {
      margin: 0;
      font-family: "Segoe UI", sans-serif;
      background: #2a5298;
      color: white;
     }
     ul {
       background-color: rgba(255, 255, 255, 0.1);
       list-style-type: none;
       margin: 0;
       padding: 0;
       overflow: hidden;
       display: flex;
     }

     li a {
       display: block;
       color: white;
       text-align: center;
       padding: 14px 16px;
       text-decoration: none;
       transition: color 0.3s;
  	   cursor: pointer;
     }

     li a:hover {
       background-color: #d4ac0d;
       color: black;
	 }

	 .titre1 {
	   color: white;
	   font-size:200%;
	   text-transform: none;
	   font-family: verdana;
	   background-color: none;
	   text-align:center;
	 }
     
	 .titre2 {
	   color: white;
	   font-size:175%;
	   font-family: verdana;
	   background-color: none;
	   text-align:center;
	 }

	 .button {
	  border: none;
	  color: white;
	  padding: 12px 28px;
	  text-align: center;
	  text-decoration: none;
	  display: inline-block;
	  font-size: 40px;
	  margin: 6px 4px;
	  transition-duration: 0.4s;
	  cursor: pointer;
	  border-radius: 12px;
	}

/*	Bouton interactif*/
	.start-btn {
	  background-color: white; 
	  color: #04AA6D; 
	  border: 4px solid #04AA6D;
	}
	.start-btn:hover {
	  background-color: #04AA6D;
	  color: white;
	}
	.stop-btn {
	  background-color: white; 
	  color: #F44336; 
	  border: 4px solid #F44336;
	}
	.stop-btn:hover {
	  background-color: #F44336;
	  color: white;
	}

	/* Tableau de position sur chaque inducteur */
	.inducteurs-colonnes {
	  display: flex;
	  justify-content: center;
	  gap: 30px; /* Réduit l'espacement global entre les colonnes */
	  margin: 20px auto;
	  font-family: Verdana, sans-serif;
	  color: white;
	  flex-wrap: wrap; /* Permet un retour à la ligne si vraiment trop petit */
	}

	.bloc-colonne {
	  display: flex;
	  flex-direction: column;
	  align-items: center;
	}

	.entete {
	  font-weight: bold;
	  font-size: 18px; /* Légèrement réduit */
	  margin-bottom: 10px;
	  text-align: center;
	}

	.valeur {
	  font-size: 16px; /* Réduit pour éviter le débordement */
	  background-color: rgba(255, 255, 255, 0.1);
	  border: 1px solid white;
	  border-radius: 6px;
	  padding: 4px 8px; /* Marges intérieures réduites */
	  margin: 4px 0;
	  width: 65px; /* Largeur réduite */
	  text-align: center;
	}

	/* Adaptation spécifique pour les smartphones */
	@media (max-width: 600px) {
	  .inducteurs-colonnes {
		gap: 10px; /* Espacement minime sur mobile */
	  }
	  .entete {
		font-size: 14px;
	  }
	  .valeur {
		font-size: 14px;
		width: 55px; /* Cellules plus fines */
		padding: 2px 4px;
	  }
	}
/*Pour l'affichage des différentes pages*/    
    .page {
    display: none;
    }

    .active {
      display: block;
    }
      .container {
      max-width: 900px;
      margin: 40px auto;
      padding: 0 16px;
      text-align: center;
    }
	</style>
</head>

<body>
<!-- Bar menu en haut -->
  <ul>
    <li><a onclick="afficherPage('page-home')">Home</a></li>
    <li><a onclick="afficherPage('page-start')">Mise en marche</a></li>
    <li><a onclick="afficherPage('page-about')">À propos</a></li>
  </ul>
  
<div class="container">
    <!-- Page Fichier -->
    <div id="page-home" class="page active">
      <h1 class="titre1">Sustentateur magnétique par attraction</h1>
    </div>

    <!-- Page Mise en marche -->
    <div id="page-start" class="page">
 
      <!-- Affiche l'état du bouton, utile pour plus tard -->
      <h2 class="titre2">État de du démonstrateur : <span id="etatBouton">À l'arrêt</span></h2> 
      <br>

      <div class="inducteurs-colonnes">
        <div class="bloc-colonne">
          <div class="entete">Inducteur n°</div>
          <div class="valeur">1</div>
          <div class="valeur">2</div>
          <div class="valeur">3</div>
          <div class="valeur">4</div>
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

      <br>
      <h3 style="color: white; margin-bottom: 15px; font-size: 18px; text-align: center;">Positions des inducteurs en temps réel</h3>
      
      <div style="width: 100%; max-width: 800px; margin: auto; background-color: rgba(255, 255, 255, 0.05); border: 1px solid rgba(255, 255, 255, 0.2); border-radius: 10px; padding: 20px; box-sizing: border-box;">
        
        <canvas id="posGraph" width="800" height="400" style="width: 100%; height: auto; display: block;"></canvas>
        
        <div style="display: flex; justify-content: center; gap: 20px; margin-top: 15px; font-size: 14px; color: white; flex-wrap: wrap;">
           <div><span style="display: inline-block; width: 25px; height: 3px; background-color: #ff4d4d; vertical-align: middle; margin-right: 8px;"></span>Inducteur 1</div>
           <div><span style="display: inline-block; width: 25px; height: 3px; background-color: #00e5ff; vertical-align: middle; margin-right: 8px;"></span>Inducteur 2</div>
           <div><span style="display: inline-block; width: 25px; height: 3px; background-color: #2962ff; vertical-align: middle; margin-right: 8px;"></span>Inducteur 3</div>
           <div><span style="display: inline-block; width: 25px; height: 3px; background-color: #ff9100; vertical-align: middle; margin-right: 8px;"></span>Inducteur 4</div>
        </div>
        
      </div>
      <br>

      <!-- Bouton interactif pour lancer / stopper la sustentation -->
      <div style="text-align:center;">
      <input type="button" id="mixBut" value="Start" data-state="0" class="button start-btn" title="Lancer la sustentation"/>
      </div>
    </div>

    <!-- Page Aide -->
    <div id="page-about" class="page">
      <h1>Contexte</h1>
      <p>La page internet que vous utilisez acutellement fait partie d'un projet de bachelor et sert principalement comme moyen de démarrer le processus d'une maquette à distance et à observer des valeurs en continu.</p>
      
      <h2>Mise en service</h2>
      <p>La procédure de lancement de la maquette en question, à l'heure actuelle, se fait dans cet ordre :</p>
      <p>1) Allumer l'alimentation 48V avec limitation de courant à 3.5A,</p>
      <p>2) Compiler la dernière version du code de Code Composer sur le DSP,</p>
      <p>3) Brancher l'oscilloscope sur la résistance de charge des cartes de puissances avec 50 ms / div et 10V / div pour les 2 sondes. Placer le trigger du canal ayant le côté électrique de la résistance vers les cartes de puissance à 20V. Mettre l'oscilloscope en "single" et observer la charge des condensateurs sur l'écran dès l'enclenchement du switch S1. </p>
      <p>4) Une fois la tension 48V établie et le transistor commuté, appuyer soit sur le bouton S2 soit sur le bouton Start dans l'onglet "Mise en marche" et la maquette devrait lancer la sustentation. </p>
      
      <br>
         
      <small> Autheur de cette page web : Thomas Freyche </small>
      <br>
      <small> v1.5 </small>
    </div>
  </div>
  
<!-- Javasript -->
<script type="text/javascript">
// Doit être **hors** de window.onload !
function afficherPage(id) {
  const pages = document.querySelectorAll('.page');
  pages.forEach(p => p.style.display = 'none');
  document.getElementById(id).style.display = 'block';
}
</script>

<script type="text/javascript">
// Doit être hors de window.onload !
function afficherPage(id) {
  const pages = document.querySelectorAll('.page');
  pages.forEach(p => p.style.display = 'none');
  document.getElementById(id).style.display = 'block';
}

window.onload = function() {
    var xmlHttp = createXmlHttpObject();
    var processInterval = null;

    var mixBut = document.getElementById("mixBut");
    var etatSpan = document.getElementById("etatBouton");

    mixBut.addEventListener("click", Start);

    function createXmlHttpObject(){
      if(window.XMLHttpRequest) return new XMLHttpRequest();
      else return new ActiveXObject("Microsoft.XMLHTTP");
    }

    function updateEtatText(etat) {
      etatSpan.textContent = etat === "1" ? "En mouvement" : "À l'arrêt";
    }

    // ==========================================
    // GESTION DU GRAPHIQUE (CANVAS)
    // ==========================================
    var histP1 = [], histP2 = [], histP3 = [], histP4 = [], timeData = [];
    var startTime = Date.now();
    var windowSize = 10.0; // Fenêtre temporelle fixe de 10 secondes

    function renderGraph() {
        var canvas = document.getElementById("posGraph");
        if(!canvas) return;
        var ctx = canvas.getContext("2d");
        var w = canvas.width;
        var h = canvas.height;

        // Effacer entièrement la zone de dessin
        ctx.clearRect(0, 0, w, h);

        var padL = 70, padB = 60, padT = 20, padR = 20;
        var plotW = w - padL - padR;
        var plotH = h - padT - padB;
        var maxY = 3.0; // Échelle max fixée à 3.0 mm

        // 1. Grille Y (Amplitude Position)
        ctx.fillStyle = "white";
        ctx.font = "14px Verdana";
        ctx.textAlign = "right";
        ctx.textBaseline = "middle";

        for(var i=0; i<=5; i++) {
            var val = i * (maxY / 5.0);
            var y = padT + plotH - (i/5.0)*plotH;
            ctx.strokeStyle = "rgba(255, 255, 255, 0.15)";
            ctx.lineWidth = 1;
            ctx.beginPath(); ctx.moveTo(padL, y); ctx.lineTo(w-padR, y); ctx.stroke();
            ctx.fillText(val.toFixed(1), padL - 10, y);
        }

        // 2. Logique de l'axe X (Temps réel)
        var tStart = 0;
        var tEnd = windowSize;

        if (timeData.length > 0) {
            var currentMaxTime = timeData[timeData.length - 1];
            if (currentMaxTime > windowSize) {
                tEnd = currentMaxTime;
                tStart = currentMaxTime - windowSize;
            }
        }

        var tSpan = tEnd - tStart;
        if (tSpan <= 0) tSpan = 1; // Sécurité mathématique

        // 3. Grille X (Ecoulement du temps)
        ctx.textAlign = "center";
        ctx.textBaseline = "top";
        var numXLabels = 5;

        for(var j=0; j<=numXLabels; j++) {
            var x = padL + (j/numXLabels)*plotW;
            ctx.strokeStyle = "rgba(255, 255, 255, 0.15)";
            ctx.beginPath(); ctx.moveTo(x, padT); ctx.lineTo(x, h-padB); ctx.stroke();

            var tVal = tStart + (j/numXLabels)*tSpan;
            ctx.fillText(tVal.toFixed(1) + "s", x, h - padB + 10);
        }

        // 4. Mots (Labels) des axes
        ctx.save();
        ctx.translate(padL - 50, padT + plotH/2);
        ctx.rotate(-Math.PI/2);
        ctx.textAlign = "center";
        ctx.fillText("Position [mm]", 0, 0);
        ctx.restore();
        ctx.fillText("Temps [s]", padL + plotW/2, h - padB + 35);

        // 5. Zone de masque (pour empêcher les lignes de sortir du cadre)
        ctx.save();
        ctx.beginPath();
        ctx.rect(padL, padT, plotW, plotH);
        ctx.clip(); 

        // 6. Tracé des courbes lié au timestamp réel
        function traceCurve(data, color) {
            if (data.length === 0) return;
            ctx.beginPath();
            ctx.strokeStyle = color;
            ctx.lineWidth = 3;
            ctx.lineJoin = "round";
            for(var k=0; k<data.length; k++) {
                // L'espacement en X dépend directement de l'heure à laquelle la donnée a été reçue !
                var px = padL + ((timeData[k] - tStart) / tSpan) * plotW;
                var valY = Math.min(Math.max(data[k], 0), maxY);
                var py = padT + plotH - (valY / maxY) * plotH;
                
                if(k===0) ctx.moveTo(px, py);
                else ctx.lineTo(px, py);
            }
            ctx.stroke();
        }

        traceCurve(histP1, "#ff4d4d");
        traceCurve(histP2, "#00e5ff");
        traceCurve(histP3, "#2962ff");
        traceCurve(histP4, "#ff9100");

        ctx.restore(); // Fin de la zone masque
    }

    // On efface les données et on reset l'heure au clic sur Start
    function resetGraph() {
        histP1 = []; histP2 = []; histP3 = []; histP4 = []; timeData = [];
        startTime = Date.now();
        renderGraph();
    }

    // Appel initial pour dessiner l'interface au chargement de la page
    renderGraph();

    // ==========================================
    // GESTION DU BOUTON START/STOP
    // ==========================================
    function Start(){
      console.log("Started");
      var xhttp = new XMLHttpRequest();

      mixBut.removeEventListener("click", Start);
      mixBut.addEventListener("click", Stop);
      mixBut.value = "Stop";
      mixBut.classList.remove("start-btn");
      mixBut.classList.add("stop-btn");
      mixBut.title = "Arrêter la sustentation"; 
      mixBut.dataset.state = "1";  
      updateEtatText("1");

      resetGraph(); // Le chrono et les tracés repartent à 0
      
      xhttp.open("PUT", "BUTTON_START", false);
      xhttp.send();
      
      processInterval = setInterval(process, 100);
    }

    function Stop(){
      console.log("Stopped");
      var xhttp = new XMLHttpRequest();

      mixBut.removeEventListener("click", Stop);
      mixBut.addEventListener("click", Start);
      mixBut.value = "Start";
      mixBut.classList.remove("stop-btn");
      mixBut.classList.add("start-btn");
      mixBut.title = "Lancer la sustentation"; 
      mixBut.dataset.state = "0";  
      updateEtatText("0");

      xhttp.open("PUT", "BUTTON_STOP", false);
      xhttp.send();
      
      if (processInterval) {
        clearInterval(processInterval);
        processInterval = null;
      }
    }

    // ==========================================
    // RECEPTION DES DONNEES (AJAX)
    // ==========================================
    function response() {
      // Sécurité anti-crash pour éviter les erreurs de lecture
      if (xmlHttp.readyState != 4) return;

      var xmlResponse = xmlHttp.responseXML;
      if(!xmlResponse) return;

      var message, xmldoc;
      
      // -- Extraction des positions --
      xmldoc = xmlResponse.getElementsByTagName("valInd1");
      if (xmldoc.length > 0) {
        message = xmldoc[0].firstChild.nodeValue.replace(',', '.');
        document.getElementById("valInd1").innerHTML = parseFloat(message).toFixed(3);
      }
      xmldoc = xmlResponse.getElementsByTagName("valInd2");
      if (xmldoc.length > 0) {
        message = xmldoc[0].firstChild.nodeValue.replace(',', '.');
        document.getElementById("valInd2").innerHTML = parseFloat(message).toFixed(3);
      }
      xmldoc = xmlResponse.getElementsByTagName("valInd3");
      if (xmldoc.length > 0) {
        message = xmldoc[0].firstChild.nodeValue.replace(',', '.');
        document.getElementById("valInd3").innerHTML = parseFloat(message).toFixed(3);
      }
      xmldoc = xmlResponse.getElementsByTagName("valInd4");
      if (xmldoc.length > 0) {
        message = xmldoc[0].firstChild.nodeValue.replace(',', '.');
        document.getElementById("valInd4").innerHTML = parseFloat(message).toFixed(3);
      }

      // -- Extraction des courants --
      xmldoc = xmlResponse.getElementsByTagName("curInd1");
      if (xmldoc.length > 0) {
        message = xmldoc[0].firstChild.nodeValue.replace(',', '.');
        document.getElementById("curInd1").innerHTML = parseFloat(message).toFixed(3);
      }
      xmldoc = xmlResponse.getElementsByTagName("curInd2");
      if (xmldoc.length > 0) {
        message = xmldoc[0].firstChild.nodeValue.replace(',', '.');
        document.getElementById("curInd2").innerHTML = parseFloat(message).toFixed(3);
      }
      xmldoc = xmlResponse.getElementsByTagName("curInd3");
      if (xmldoc.length > 0) {
        message = xmldoc[0].firstChild.nodeValue.replace(',', '.');
        document.getElementById("curInd3").innerHTML = parseFloat(message).toFixed(3);
      }
      xmldoc = xmlResponse.getElementsByTagName("curInd4");
      if (xmldoc.length > 0) {
        message = xmldoc[0].firstChild.nodeValue.replace(',', '.');
        document.getElementById("curInd4").innerHTML = parseFloat(message).toFixed(3);
      }

      // -- Ajout des données au graphique en temps réel --
      var p1 = parseFloat(document.getElementById("valInd1").innerHTML);
      var p2 = parseFloat(document.getElementById("valInd2").innerHTML);
      var p3 = parseFloat(document.getElementById("valInd3").innerHTML);
      var p4 = parseFloat(document.getElementById("valInd4").innerHTML);

      var currentTime = (Date.now() - startTime) / 1000.0;
      timeData.push(currentTime);
      histP1.push(p1);
      histP2.push(p2);
      histP3.push(p3);
      histP4.push(p4);

      // Le script supprime les points qui sont plus vieux que la fenêtre de 10 secondes
      while(timeData.length > 0 && (currentTime - timeData[0] > windowSize)) {
          timeData.shift();
          histP1.shift();
          histP2.shift();
          histP3.shift();
          histP4.shift();
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