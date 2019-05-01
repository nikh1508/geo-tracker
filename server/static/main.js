var url = "./api/get_latest_data";
var map;
var marker;
var xmlhttp = new XMLHttpRequest();

xmlhttp.onreadystatechange = function () {
    if (this.readyState == 4 && this.status == 200) {
        data = JSON.parse(this.responseText);
    }
};
xmlhttp.open("GET", url, true);
xmlhttp.send();

function initMap() {
    var myLatLng = { lat: data["Latitude"], lng: data["Longitude"] };
    map = new google.maps.Map(document.getElementById('map'), {
        zoom: 17,
        center: myLatLng
    });

    marker = new google.maps.Marker({
        position: myLatLng,
        map: map,
        title: 'Current Location'
    });
}

setInterval(function() {
    xmlhttp.open("GET", url, true);
    xmlhttp.send();
    var myLatLng = { lat: data["Latitude"], lng: data["Longitude"] };
    map.setCenter(myLatLng);
    marker.setPosition(myLatLng);
    var info = "";
    for(key in data){
        info += "<h4>"+ key + " : " + data[key] + "\n";
    }
    document.getElementById('info').innerHTML = info;
}, 5000)
