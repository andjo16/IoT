async function drawTempChart() {
	let tempChart = document.getElementById("tempChart");

	let tempData = await getTemp();
	let plotData = [
		{
			x: tempData["timestamp_1"],
			y: tempData["temperature_1"],
			line: {shape: 'spline'},
			name: "Temp 1"
		},
		{
			x: tempData["timestamp_2"],
			y: tempData["temperature_2"],
			line: {shape: 'spline'},
			name: "Temp 2"
		}
	];

	let plotLayout = {
		yaxis: {
			range: [0, 100],
			autorange: false
		}
	}

	Plotly.newPlot(
		tempChart,
		plotData,
		plotLayout,
		{
			margin: { t: 2 }
		}
	);
}

async function printNumberPeople() {
	let div = document.getElementById("numberPeople");
	let rsp = await getNumberPeople();
	div.innerHTML = `${rsp["number_people"]}`;
}

const VENTILATION_ID = 3
const WORKSHOP_LIGHT_ID = 5;
const LOUNGE_LIGHT_ID = 6;
const STATE_ON = 1;
const STATE_OFF = 0;

async function lightStatus(obj) {
	let workshopText = document.getElementById("workshopState");
	let workshopButton = document.getElementById("workshopButton");
	let loungeText = document.getElementById("loungeState");
	let laucnhButton = document.getElementById("loungeButton");
	let rsp = await getLights();
	if (rsp["state_1"]) {
		workshopText.innerHTML = "ON";
		workshopText.style.color = "green";
		workshopButton.innerHTML = "Turn off light";
		workshopButton.disabled = false;
		workshopButton.onclick = () => {
			workshopButton.disabled = true;
			setState(WORKSHOP_LIGHT_ID, STATE_OFF);
		};
	} else {
		workshopText.innerHTML = "OFF";
		workshopText.style.color = "red";
		workshopButton.innerHTML = "Turn on light";
		workshopButton.disabled = false;
		workshopButton.onclick = () => {
			workshopButton.disabled = true;
			setState(WORKSHOP_LIGHT_ID, STATE_ON);
		};
	}
	if (rsp["state_2"]) {
		loungeText.innerHTML = "ON";
		loungeText.style.color = "green";
		loungeButton.innerHTML = "Turn off light";
		loungeButton.disabled = false;
		loungeButton.onclick = () => {
			loungeButton.disabled = true;
			setState(LOUNGE_LIGHT_ID, STATE_OFF)
		};
	} else {
		loungeText.innerHTML = "OFF";
		loungeText.style.color = "red";
		loungeButton.innerHTML = "Turn on light";
		loungeButton.disabled = false;
		loungeButton.onclick = () => {
			loungeButton.disabled = true;
			setState(LOUNGE_LIGHT_ID, STATE_ON)
		};
	}
}

async function ventilationStatus() {
	let text = document.getElementById("ventilationState");
	let button = document.getElementById("ventilationButton");
	let rsp = await getVentilation();
	if (rsp["state"]) {
		text.innerHTML = "ON";
		text.style.color = "green";
		button.innerHTML = "Turn off ventilation";
		button.disabled = false;
		button.onclick = () => {
			button.disabled = true;
			setState(VENTILATION_ID, STATE_OFF)
		};
	} else {
		text.innerHTML = "OFF";
		text.style.color = "red";
		button.innerHTML = "Turn on ventilation";
		button.disabled = false;
		button.onclick = () => {
			button.disabled = true;
			setState(VENTILATION_ID, STATE_ON)
		};
	}
}

function renderData() {
	drawTempChart();
	printNumberPeople();
	lightStatus();
	ventilationStatus();
}

renderData();
setInterval(renderData, 5000);
