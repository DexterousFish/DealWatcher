// ebay_search.js - search active eBay listings with the Browse API.
//
// Requires Node.js 18+ (fetch is built in; no packages to install).
//
// Run:
//   export EBAY_CLIENT_ID="your App ID"
//   export EBAY_CLIENT_SECRET="your Cert ID"
//   node ebay_search.js super mario world snes
//
// Optional environment variables:
//   EBAY_ENV=production   use api.ebay.com + production keys (default: sandbox)
//   MAX_PRICE=30          only show listings at or under this USD price

const ENV = process.env.EBAY_ENV === "production" ? "production" : "sandbox";
const API_HOST =
  ENV === "production" ? "https://api.ebay.com" : "https://api.sandbox.ebay.com";

// Step 1: trade client ID + secret for a temporary access token.
async function getToken(clientId, clientSecret) {
  const basic = Buffer.from(`${clientId}:${clientSecret}`).toString("base64");

  const res = await fetch(`${API_HOST}/identity/v1/oauth2/token`, {
    method: "POST",
    headers: {
      "Content-Type": "application/x-www-form-urlencoded",
      Authorization: `Basic ${basic}`,
    },
    body: new URLSearchParams({
      grant_type: "client_credentials",
      scope: "https://api.ebay.com/oauth/api_scope",
    }),
  });

  const data = await res.json();
  if (!res.ok) {
    throw new Error(`Token request failed (${res.status}): ${JSON.stringify(data)}`);
  }
  return data.access_token;
}

// Step 2: use the token to search active listings.
async function searchItems(token, query, { limit = 10, maxPrice } = {}) {
  const params = new URLSearchParams({ q: query, limit: String(limit) });
  if (maxPrice) {
    params.set("filter", `price:[..${maxPrice}],priceCurrency:USD`);
  }

  const res = await fetch(`${API_HOST}/buy/browse/v1/item_summary/search?${params}`, {
    headers: {
      Authorization: `Bearer ${token}`,
      "X-EBAY-C-MARKETPLACE-ID": "EBAY_US",
    },
  });

  const data = await res.json();
  if (!res.ok) {
    throw new Error(`Search failed (${res.status}): ${JSON.stringify(data)}`);
  }
  return data;
}

async function main() {
  const clientId = process.env.EBAY_CLIENT_ID;
  const clientSecret = process.env.EBAY_CLIENT_SECRET;
  // Joining the arguments means quotes are optional: both
  //   node ebay_search.js super mario world snes
  //   node ebay_search.js "super mario world snes"
  // search for the whole phrase.
  const query = process.argv.slice(2).join(" ");

  if (!clientId || !clientSecret || !query) {
    console.error('Usage: EBAY_CLIENT_ID=... EBAY_CLIENT_SECRET=... node ebay_search.js "search terms"');
    process.exit(1);
  }

  const token = await getToken(clientId, clientSecret);
  const result = await searchItems(token, query, {
    limit: 10,
    maxPrice: process.env.MAX_PRICE,
  });

  const items = result.itemSummaries ?? []; // missing when nothing matched
  console.log(`[${ENV}] "${query}": ${result.total ?? 0} matches, showing ${items.length}\n`);

  for (const item of items) {
    const price = `${item.price.value} ${item.price.currency}`;
    const shipping = item.shippingOptions?.[0]?.shippingCost?.value;
    const shipText = shipping === undefined ? "shipping unknown" : `+$${shipping} shipping`;

    console.log(`${price} (${shipText}) | ${item.condition ?? "n/a"}`);
    console.log(`  ${item.title}`);
    console.log(`  ${item.itemWebUrl}\n`);
  }
}

main().catch((err) => {
  console.error(err.message);
  process.exit(1);
});