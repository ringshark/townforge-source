// Use the existing Cloud save project; never trust a client-supplied account ID.
export async function verifyLadderAccount(env,token,request=fetch) {
  if(typeof token!=='string' || token.length>8192 || !/^[A-Za-z0-9_-]+\.[A-Za-z0-9_-]+\.[A-Za-z0-9_-]+$/.test(token)) throw Error('Sign in with Cloud save to enter the weekly ladder.');
  const url=String(env.SUPABASE_URL || '').replace(/\/$/,'');
  if(!/^https:\/\/[a-z0-9-]+\.supabase\.co$/.test(url) || !env.SUPABASE_ANON_KEY) throw Error('The ladder account service is not configured.');
  let response;
  try {response=await request(url+'/auth/v1/user',{headers:{apikey:env.SUPABASE_ANON_KEY,Authorization:'Bearer '+token},signal:AbortSignal.timeout(8000)});}catch(e) {throw Error('Account verification unavailable. Try again shortly.');}
  if(!response.ok) throw Error('Cloud sign-in expired. Sign in again before entering the ladder.');
  const user=await response.json();
  if(!/^[a-f0-9]{8}-[a-f0-9]{4}-[a-f0-9]{4}-[a-f0-9]{4}-[a-f0-9]{12}$/i.test(user.id || '')) throw Error('Account verification failed.');
  return user.id;
}
