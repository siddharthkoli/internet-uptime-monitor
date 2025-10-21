// Setup type definitions for built-in Supabase Runtime APIs
import "jsr:@supabase/functions-js/edge-runtime.d.ts";
import { createClient } from 'jsr:@supabase/supabase-js@2';
Deno.serve(async (req)=>{
  if (req.method !== "POST") {
    return new Response("Method Not Allowed", {
      status: 405
    });
  }
  const body = await req.json(); // Could be object or array
  const IST_OFFSET_MS = 5.5 * 60 * 60 * 1000;
  const now = new Date(Date.now() + IST_OFFSET_MS);
  const currentISTDateStart = new Date(now.getFullYear(), now.getMonth(), now.getDate());
  // Helper to compute asofdate for a single object
  const computeAsofDate = (obj)=>{
    let objDate = new Date(currentISTDateStart.getTime() + obj.minute * 60 * 1000);
    if (objDate > now) objDate.setDate(objDate.getDate() - 1);
    const dd = String(objDate.getDate()).padStart(2, '0');
    const mm = String(objDate.getMonth() + 1).padStart(2, '0');
    const yyyy = objDate.getFullYear();
    return {
      ...obj,
      asofdate: `${dd}-${mm}-${yyyy}`
    };
  };
  // Normalize to array
  const bodyArray = Array.isArray(body) ? body : [
    body
  ];
  // Compute asofdate for each object
  const bodyWithDate = bodyArray.map(computeAsofDate);
  const supabase = createClient(Deno.env.get("SUPABASE_URL"), Deno.env.get("SUPABASE_SERVICE_ROLE_KEY"));
  // Insert all logs, idempotent via upsert
  const { data, error } = await supabase.from("wifi_logs").upsert(bodyWithDate, {
    onConflict: [
      'device_id',
      'minute',
      'asofdate'
    ]
  });
  if (error) {
    console.error(`Error while inserting data into DB: ${error.message}`);
    return new Response(JSON.stringify(error), {
      status: 400
    });
  }
  return new Response(JSON.stringify(data));
});
