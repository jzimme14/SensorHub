// SIDEBAR TOGGLE

let sidebarOpen = false;
const sidebar = document.getElementById('sidebar');
let transponderId = 1;

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
  let temperatureElement = document.getElementById('temperature');
  temperatureElement.textContent = data.temperature;

  let humidityElement = document.getElementById('humidity');
  humidityElement.textContent = data.humidity;
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
  })
  .then(data => {
      // Update the UI with the fetched data
      updateUI(data);
  })
  .catch(error => {
      console.error('There was a problem with the fetch operation:', error);
  });
}
