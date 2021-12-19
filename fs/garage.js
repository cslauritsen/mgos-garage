
  function load_data() {
    $(document).ready(function() {

      $.get("/rpc/status2.read.json", function(data, status) {
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

        for (var i=0; i < data.doors.length; i++) {
          var d = data.doors[i];
          console.log('iterating door ' + i);
          var div = $('#door_template').clone().prop('id', 'door-' + i);
          div.appendTo('#body');

          div.find('h2').text(d.name);
          div.find('.statusdiv').text('Status: ' + d.status);
          div.find('.statusdiv').css("background-color", d.status === "closed" ? "green" : "pink");

          let doorLetter = String.fromCharCode(i + ('a'.charCodeAt(0)));
          console.log('Door ' + i + ' letter: ' + doorLetter);
          let nm = d.name;
          div.find('.actionbtn').click(function() {
            $('<div class="alert alert-success">' + nm + ' activated!</div>').insertBefore('#topdiv').delay(3000).fadeOut();
            $.get("/rpc/door" + doorLetter + ".activate", function(data, status){
              console.log("Activate Result: " + data.value + "\nStatus: " + status);
            });
            return false;
          });
        }
      });

    });


  }