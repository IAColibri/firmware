// Generated by CoffeeScript 1.10.0
(function() {
  update = function() {
    sensor = new HttpClient();
    sensor.get("/sensor.md", function(response) {
	status_sensor = document.getElementsByClassName("status_door");
	for(var i = 0; i < status_sensor.length; i++){
		status_sensor[i].innerText= response;
	}
    });
    button = new HttpClient();
    button.get("/button.md", function(response) {
	status_button = document.getElementsByClassName("status_button");
	for(var i = 0; i < status_button.length; i++){
		status_button[i].innerText= response;
	}
    });
  };
}).call(this);

var HttpClient = function() {
    this.get = function(aUrl, aCallback) {
        var anHttpRequest = new XMLHttpRequest();
        anHttpRequest.onreadystatechange = function() { 
            if (anHttpRequest.readyState == 4 && anHttpRequest.status == 200)
                aCallback(anHttpRequest.responseText);
        }
        anHttpRequest.open( "GET", aUrl, true );            
        anHttpRequest.send( null );
    }
}
function push_button() {
	push_button = new HttpClient();
	push_button.get("/push_button.html", function(response) {
	status_button = document.getElementsByClassName("response");
		for(var i = 0; i < status_button.length; i++){
			status_button[i].innerText= response;
		}
	});

}
setInterval(update, 1000 * 60 * 1);
