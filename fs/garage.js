
  function load_data() {
    $(document).ready(function() {

      $.get("/rpc/status.read.json", function(data, status) {
        cell = $("#tempf");
        if (status === "success") {
          cell.text(data.tempf.toFixed(1) + "°F");
        }
        else {
          cell.text(status);
          cell.css("background-color", "yellow");
        }

        cell = $("#rh");
        if (status === "success") {
          cell.text(data.rh.toFixed(1) + "%");
        }
        else {
          cell.text(status);
          cell.css("background-color", "yellow");
        }
        $("#h2_door_a").text(data.doors[0].name);
        $("#status_a").text(data.doors[0].status);
        $("#status_a").css("background-color", 
          data.doors[0].status === "closed" ? "green" : "red");
        $("#h2_door_b").text(data.doors[1].name);
        $("#status_b").text(data.doors[1].status);
        $("#status_b").css("background-color", 
          data.doors[1].status === "closed" ? "green" : "red");
        $("#now").text(data.currentTime);
      });

    });

    $('#btnactivate_a').click(function() {
      $.get("/rpc/doora.activate", function(data, status){
        alert("Closing Result: " + data.value + "\nStatus: " + status);
      });
      return false;
    });

    $('#btnactivate_b').click(function() {
      $.get("/rpc/doorb.activate", function(data, status){
        alert("Closing Result: " + data.value + "\nStatus: " + status);
      });
      return false;
    });
  }