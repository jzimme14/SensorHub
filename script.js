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
  temperatureElement.textContent = data;

  let humidityElement = document.getElementById('hum_text');
  humidityElement.textContent = data;

  let pressureElement = document.getElementById('pressure_text');
  humidityElement.textContent = data;

  let timeElement = document.getElementById('time_text');
  humidityElement.textContent = data;
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
      console.log(response,response.text(),response.arrayBuffer());
      
  })
  .then(data => {
      console.log(data);
      console.log(Promise.resolve());
      const dataArray = data.split(',');
      console.log(dataArray);
  })
  .catch(error => {
      console.error('ERROR', error);
  });
}
