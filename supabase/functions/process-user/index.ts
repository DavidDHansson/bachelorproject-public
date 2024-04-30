import * as postgres from "postgres"

const databaseUrl = Deno.env.get('DB_URL')!
const pool = new postgres.Pool(databaseUrl, 3, true)

// Called by card to box and from phone

Deno.serve(async (_req) => {

  try {
    const { userId, locationId, invalidate, addToTimesCheckedIn } = await _req.json()

    if (userId === undefined || locationId === undefined) {
      return new Response(String("No userId or locationId given"), { status: 422 })
    }

    let statusMsg = ""
    let status = "failure"

    // Update database
    const connection = await pool.connect()

    try {
      
      const data = await connection.queryObject`
        SELECT a.title, a.description, u.first_name, u.last_name, l.name
        FROM appointments a
        INNER JOIN users u ON a.user_id = u.id
        INNER JOIN locations l ON a.location_id = l.id WHERE a.user_id = ${userId} AND a.location_id = ${locationId};
      `
      const appointment = data.rows[0]
      const title = appointment.title
      const description = appointment.description
      const userName = appointment.first_name + " " + appointment.last_name
      const locationName = appointment.name

      statusMsg = userName

      // Both
      if (addToTimesCheckedIn != undefined && addToTimesCheckedIn > 0 && invalidate != undefined && invalidate) {
        const result = await connection.queryObject`UPDATE appointments SET times_checked_in = times_checked_in + ${addToTimesCheckedIn}, is_valid = 'FALSE' WHERE user_id = ${userId} AND location_id = ${locationId}`
        status = "success"
      } else {
        // Add
        if (addToTimesCheckedIn != undefined && addToTimesCheckedIn > 0) {
          const result = await connection.queryObject`UPDATE appointments SET times_checked_in = times_checked_in + ${addToTimesCheckedIn} WHERE user_id = ${userId} AND location_id = ${locationId}`
          status = "success"
        }
  
        // Invalidate
        if(invalidate != undefined && invalidate) {
          const result = await connection.queryObject`UPDATE appointments SET is_valid = 'FALSE' WHERE user_id = ${userId} AND location_id = ${locationId}`
          status = "success"
        }
      }
    } catch (error) {
      statusMsg = "Failed to check in, please try again or contact staff";
      connection.release()
    } finally {
      connection.release()
    } 

    // Call to MQTT to notify box 
    const data = {
      topic: 'emqx/esp32/' + locationId,
      qos: 1,
      payload: status + " " + statusMsg
    }

    const response = await fetch('', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
        'Authorization': "Basic "
      },
      body: JSON.stringify(data)
    })

    if (!response.ok) {
      throw new Error('Network response was not ok')
    }

    const jsonResponse = await response.json();
    console.log(jsonResponse);

    return new Response("", { status: 200 })
  } catch (error) {
    console.error('Error:', error);
    return new Response(String(error?.message ?? error), { status: 500 })
  }

  return new Response("", {
    status: 200,
    headers: { 'Content-Type': 'application/json charset=utf-8' },
  })

})
