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

/*Tableau de position sur chaque inducteur*/
	.inducteurs-colonnes {
	  display: flex;
	  justify-content: center;
	  gap: 60px;
	  margin: 30px auto;
	  font-family: Verdana, sans-serif;
	  color: white;
	}

	.bloc-colonne {
	  display: flex;
	  flex-direction: column;
	  align-items: center;
	}

	.entete {
	  font-weight: bold;
	  font-size: 20px;
	  margin-bottom: 10px;
	}

	.valeur {
	  font-size: 18px;
	  background-color: rgba(255, 255, 255, 0.1);
	  border: 1px solid white;
	  border-radius: 6px;
	  padding: 6px 16px;
	  margin: 4px 0;
	  width: 80px;
	  text-align: center;
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
window.onload = function() {
	// global variable visible to all java functions	
    var xmlHttp=createXmlHttpObject();
    var processInterval = null; // Variable pour contrôler la mise à jour

    var mixBut = document.getElementById("mixBut");
    var etatSpan = document.getElementById("etatBouton"); // 0 = à l'arrêt, 1 = en lévitation

    mixBut.addEventListener("click", Start);
	
  	function afficherPage(id) {
      document.querySelectorAll('.page').forEach(p => p.classList.remove('active'));
      document.getElementById(id).classList.add('active');
    }
    
    // function to create XML object
    function createXmlHttpObject(){
      if(window.XMLHttpRequest){
        xmlHttp=new XMLHttpRequest();
      }
      else{
        xmlHttp=new ActiveXObject("Microsoft.XMLHTTP");
      }
      return xmlHttp;
    }

    function updateEtatText(etat) {
    etatSpan.textContent = etat === "1" ? "En mouvement" : "À l'arrêt";
	}
    
    function Start(){
      console.log("Started");
      var xhttp = new XMLHttpRequest();

      mixBut.removeEventListener("click", Start);
      mixBut.addEventListener("click", Stop);
      mixBut.value = "Stop";
      mixBut.classList.remove("start-btn");
    	mixBut.classList.add("stop-btn");
    	mixBut.title = "Arrêter la sustentation"; // Stop's tooltip 
    	mixBut.dataset.state = "1";  // état = stop
      updateEtatText("1");

      xhttp.open("PUT", "BUTTON_START", false);
      xhttp.send();
      
      // Démarrer la mise à jour des données toutes les 1 seconde
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
    	mixBut.title = "Lancer la sustentation"; // Stop's tooltip 
    	mixBut.dataset.state = "0";  // état = start
      updateEtatText("0");

      xhttp.open("PUT", "BUTTON_STOP", false);
      xhttp.send();
      
      // Arrêter la mise à jour des données
      if (processInterval) {
        clearInterval(processInterval);
        processInterval = null;
      }
    }

    // function to handle the response from the ESP
    function response(){
      var message;
      var xmlResponse;
      var xmldoc;
     
      // get the xml stream
      xmlResponse=xmlHttp.responseXML;
  
      // Mise à jour des 4 inducteurs
      // Inducteur 1
      xmldoc = xmlResponse.getElementsByTagName("valInd1");
      if (xmldoc.length > 0) {
        message = xmldoc[0].firstChild.nodeValue;
        // Remplace la virgule par un point pour parseFloat
        message = message.replace(',', '.');
        document.getElementById("valInd1").innerHTML = parseFloat(message).toFixed(3);
      }
      
      // Inducteur 2
      xmldoc = xmlResponse.getElementsByTagName("valInd2");
      if (xmldoc.length > 0) {
        message = xmldoc[0].firstChild.nodeValue;
        message = message.replace(',', '.');
        document.getElementById("valInd2").innerHTML = parseFloat(message).toFixed(3);
      }
  
      // Inducteur 3
      xmldoc = xmlResponse.getElementsByTagName("valInd3");
      if (xmldoc.length > 0) {
        message = xmldoc[0].firstChild.nodeValue;
        message = message.replace(',', '.');
        document.getElementById("valInd3").innerHTML = parseFloat(message).toFixed(3);
      }
      
      // Inducteur 4
      xmldoc = xmlResponse.getElementsByTagName("valInd4");
      if (xmldoc.length > 0) {
        message = xmldoc[0].firstChild.nodeValue;
        message = message.replace(',', '.');
        document.getElementById("valInd4").innerHTML = parseFloat(message).toFixed(3);
      }
     }
  
    // general processing code for the web page to ask for an XML steam
    function process(){
     if(xmlHttp.readyState==0 || xmlHttp.readyState==4) {
        xmlHttp.open("PUT","xml",true);
        xmlHttp.onreadystatechange=response;
        xmlHttp.send(null);
      }       
    }

};
</script>

</body>
</html>



)=====";