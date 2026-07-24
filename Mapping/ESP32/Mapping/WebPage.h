const char PAGE_MAIN[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="fr">
<head>
  <title>Mapping Plaque de Mesure</title>
  <meta charset="UTF-8">  
  <meta name="viewport" content="width=device-width, initial-scale=1.0"> 
  <style>
    body { font-family: "Segoe UI", sans-serif; background: #1e3c72; color: white; text-align: center; padding: 50px 20px; }
    .container { max-width: 600px; margin: auto; background: rgba(255,255,255,0.1); padding: 30px; border-radius: 15px; box-shadow: 0 4px 15px rgba(0,0,0,0.3); }
    h1 { font-size: 24px; margin-bottom: 10px; }
    h2 { font-size: 18px; color: #00e5ff; margin-bottom: 30px; }
    .button { border: none; color: white; padding: 15px 40px; text-align: center; font-size: 22px; margin: 10px; cursor: pointer; border-radius: 8px; transition: 0.3s; width: 80%; max-width: 300px; }
    .start-btn { background-color: #04AA6D; }
    .start-btn:hover { background-color: #038d5a; }
    .stop-btn { background-color: #F44336; }
    .stop-btn:hover { background-color: #da382c; }
    .csv-btn { background-color: #2962ff; font-size: 18px; display: none; }
    #status { font-weight: bold; margin-top: 15px; color: #ffeb3b; }
  </style>
</head>
<body>

<div class="container">
  <h1>Cartographie & Mapping</h1>
  <h2 id="etatBouton">Statut : À l'arrêt</h2>

  <input type="button" id="mixBut" value="Start" class="button start-btn"/>
  <br>
  <input type="button" id="csvBut" value="Télécharger CSV (10s)" class="button csv-btn" onclick="downloadCSV()"/>
  <p id="status"></p>
</div>

<script type="text/javascript">
var csvData = [];
var isRecording = false;
var recordStartTime = 0;
var processInterval = null;
var xmlHttp = new XMLHttpRequest();
var mixBut = document.getElementById("mixBut");
var etatSpan = document.getElementById("etatBouton");
var statusText = document.getElementById("status");

mixBut.addEventListener("click", Start);

function Start(){
  mixBut.removeEventListener("click", Start);
  mixBut.addEventListener("click", Stop);
  mixBut.value = "Stop";
  mixBut.className = "button stop-btn";
  etatSpan.textContent = "Statut : Acquisition en cours...";

  // Initialisation du tableau CSV avec vos 4 colonnes de position
  csvData = [["Temps [s]", "Pos1 [brut]", "Pos2 [brut]", "Pos3 [brut]", "Pos4 [brut]"]];
  isRecording = true;
  recordStartTime = Date.now();
  
  document.getElementById("csvBut").style.display = "none";
  statusText.innerText = "";

  fetch('/BUTTON_START', { method: 'PUT' })
    .then(() => {
        // Interrogation de l'ESP32 toutes les 15ms pour vider le buffer au plus vite
        if (!processInterval) { processInterval = setInterval(process, 15); }
    });
}

function Stop(){
  mixBut.removeEventListener("click", Stop);
  mixBut.addEventListener("click", Start);
  mixBut.value = "Start";
  mixBut.className = "button start-btn";
  etatSpan.textContent = "Statut : À l'arrêt";
  isRecording = false;

  fetch('/BUTTON_STOP', { method: 'PUT' })
    .then(() => {
        if (processInterval) { clearInterval(processInterval); processInterval = null; }
    });
}

function process() {
  if(xmlHttp.readyState==0 || xmlHttp.readyState==4) {
    xmlHttp.open("PUT", "xml", true);
    xmlHttp.onreadystatechange = function() {
      if (xmlHttp.readyState != 4) return;
      var xmlResponse = xmlHttp.responseXML;
      if(!xmlResponse) return;

      var ext = function(tag) {
          var nodes = xmlResponse.getElementsByTagName(tag);
          if (nodes.length > 0 && nodes[0].firstChild) return parseFloat(nodes[0].firstChild.nodeValue);
          return 0.0;
      };

      var p1 = ext("p1"), p2 = ext("p2"), p3 = ext("p3"), p4 = ext("p4");
      var currentTime = (Date.now() - recordStartTime) / 1000.0;

      if (isRecording) {
          csvData.push([
              currentTime.toFixed(3).replace('.', ','), 
              p1.toFixed(4).replace('.', ','),
              p2.toFixed(4).replace('.', ','), 
              p3.toFixed(4).replace('.', ','),
              p4.toFixed(4).replace('.', ',')
          ]);

          if (currentTime >= 10.0) {
              Stop();
              statusText.innerText = "Acquisition terminée !";
              document.getElementById("csvBut").style.display = "inline-block";
          }
      }
    };
    xmlHttp.send(null);
  }       
}

function downloadCSV() {
    var csvContent = csvData.map(e => e.join(";")).join("\n");
    var blob = new Blob([csvContent], { type: 'text/csv;charset=utf-8;' });
    var url = URL.createObjectURL(blob);
    var link = document.createElement("a");
    link.setAttribute("href", url);
    link.setAttribute("download", "mapping_plaque_mesure.csv");
    document.body.appendChild(link);
    link.click();
    document.body.removeChild(link);
}
</script>
</body>
</html>
)=====";