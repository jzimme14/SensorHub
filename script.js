// SIDEBAR TOGGLE

let sidebarOpen = false;
const sidebar = document.getElementById('sidebar');
let transponderId = 1;

let data;
let response;

function openSidebar() {
  if (!sidebarOpen) {
    sidebar.classList.add('sidebar-responsive');
    sidebarOpen = true;
  }
}

function closeSidebar() {
  if (sidebarOpen) {
    sidebar.classList.remove('sidebar-responsive');
    sidebarOpen = false;
  }
}

// update the UI with new data
function updateUI(data){
  console.log(data);
  let temperatureElement = document.getElementById('temp_text');
  temperatureElement.textContent = data[1];

  let humidityElement = document.getElementById('hum_text');
  humidityElement.textContent = data[2];

  let pressureElement = document.getElementById('pressure_text');
  pressureElement.textContent = data[3];

  let timeElement = document.getElementById('time_text');
  timeElement.textContent = data[4];
}

// change header text and get new data
function updateDataAndUI(transponderId){
  let url = `/measData.db?${transponderId}`;


  // change headertext-title after click on sidebar object here
  let headerobj = document.getElementById('headertext');

  switch (transponderId) {
    case 1:
      headerobj.innerHTML = 'Room (Jakob)';
      break;
    case 2:
      headerobj.innerHTML = 'Room (Luis)';
      break;
    case 3:
      headerobj.innerHTML = 'Living Room';
      break;
    case 4:
      headerobj.innerHTML = 'Outside';
      break;
    default:
      headerobj.innerHTML = 'Unknown';
      break;
  }

  fetch(`${url}`)
  .then(response => {
      if (!response.ok) {
          throw new Error('Network response was not ok');
      }
      return response.text();
  })
  .then(data => {
      const dataArray = data.split(',');
      updateUI(dataArray);
  })
  .catch(error => {
      console.error('ERROR', error);
  });
}
