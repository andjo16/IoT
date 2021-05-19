base_url = "https://dt-iotgroup6-backend.herokuapp.com";

async function getTemp() {
    let url = `${base_url}/temperature`;
    try {
        let res = await fetch(url);
        return await res.json();
    } catch (error) {
        console.log(error);
    }
}

async function getNumberPeople() {
	let url = `${base_url}/number_people`;
	try {
		let res = await fetch(url);
		return await res.json();
	} catch (error) {
		console.log(error);
	}
}

async function getLights() {
	let url = `${base_url}/lights`;
	try {
		let res = await fetch(url);
		return await res.json();
	} catch (error) {
		console.log(error);
	}
}

async function getVentilation() {
	let url = `${base_url}/ventilation`;
	try {
		let res = await fetch(url);
		return await res.json();
	} catch (error) {
		console.log(error);
	}
}

async function setState(sensor_id, state) {
	let url = `${base_url}/cibicom_downlink`;
	const options = {
		method: 'POST',
		body: `{"sensor_id": ${sensor_id}, "state": ${state}}`,
		headers: {
			'Content-Type': 'application/json'
		}
	}
	try {
		let res = await fetch(url, options)
	} catch (error) {
		console.log(error);
	}
}
